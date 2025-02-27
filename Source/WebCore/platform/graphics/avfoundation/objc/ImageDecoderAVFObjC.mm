/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "ImageDecoderAVFObjC.h"

#if HAVE(AVSAMPLEBUFFERGENERATOR)

#import "AVFoundationMIMETypeCache.h"
#import "AffineTransform.h"
#import "ContentType.h"
#import "FloatQuad.h"
#import "FloatRect.h"
#import "FloatSize.h"
#import "Logging.h"
#import "MIMETypeRegistry.h"
#import "SharedBuffer.h"
#import "UTIUtilities.h"
#import "WebCoreDecompressionSession.h"
#import <AVFoundation/AVAsset.h>
#import <AVFoundation/AVAssetResourceLoader.h>
#import <AVFoundation/AVAssetTrack.h>
#import <AVFoundation/AVSampleBufferGenerator.h>
#import <AVFoundation/AVSampleCursor.h>
#import <AVFoundation/AVTime.h>
#import <VideoToolbox/VTUtilities.h>
#import <map>
#import <pal/avfoundation/MediaTimeAVFoundation.h>
#import <wtf/MainThread.h>
#import <wtf/MediaTime.h>
#import <wtf/NeverDestroyed.h>
#import <wtf/OSObjectPtr.h>
#import <wtf/SoftLinking.h>
#import <wtf/Vector.h>

#import <pal/cf/CoreMediaSoftLink.h>
#import "VideoToolboxSoftLink.h"

#pragma mark - Soft Linking

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)
SOFT_LINK_CLASS_OPTIONAL(AVFoundation, AVURLAsset)
SOFT_LINK_CLASS_OPTIONAL(AVFoundation, AVSampleBufferGenerator)
SOFT_LINK_CLASS_OPTIONAL(AVFoundation, AVSampleBufferRequest)
SOFT_LINK_POINTER_OPTIONAL(AVFoundation, AVMediaCharacteristicVisual, NSString *)
SOFT_LINK_POINTER_OPTIONAL(AVFoundation, AVURLAssetReferenceRestrictionsKey, NSString *)
SOFT_LINK_POINTER_OPTIONAL(AVFoundation, AVURLAssetUsesNoPersistentCacheKey, NSString *)
#define AVMediaCharacteristicVisual getAVMediaCharacteristicVisual()
#define AVURLAssetReferenceRestrictionsKey getAVURLAssetReferenceRestrictionsKey()
#define AVURLAssetUsesNoPersistentCacheKey getAVURLAssetUsesNoPersistentCacheKey()

#pragma mark -

@interface WebCoreSharedBufferResourceLoaderDelegate : NSObject<AVAssetResourceLoaderDelegate> {
    WebCore::ImageDecoderAVFObjC* _parent;
    long long _expectedContentSize;
    RetainPtr<NSData> _data;
    bool _complete;
    Vector<RetainPtr<AVAssetResourceLoadingRequest>> _requests;
    Lock _dataLock;
}
- (id)initWithParent:(WebCore::ImageDecoderAVFObjC*)parent;
- (void)setExpectedContentSize:(long long)expectedContentSize;
- (void)updateData:(NSData *)data complete:(BOOL)complete;
- (BOOL)canFulfillRequest:(AVAssetResourceLoadingRequest *)loadingRequest;
- (void)enqueueRequest:(AVAssetResourceLoadingRequest *)loadingRequest;
- (void)fulfillPendingRequests;
- (void)fulfillRequest:(AVAssetResourceLoadingRequest *)loadingRequest;
@end

@implementation WebCoreSharedBufferResourceLoaderDelegate
- (id)initWithParent:(WebCore::ImageDecoderAVFObjC*)parent
{
    if (!(self = [super init]))
        return nil;
    _parent = parent;

    return self;
}

- (void)setExpectedContentSize:(long long)expectedContentSize
{
    LockHolder holder { _dataLock };
    _expectedContentSize = expectedContentSize;

    [self fulfillPendingRequests];
}

- (void)updateData:(NSData *)data complete:(BOOL)complete
{
    LockHolder holder { _dataLock };
    _data = data;
    _complete = complete;

    [self fulfillPendingRequests];
}

- (BOOL)canFulfillRequest:(AVAssetResourceLoadingRequest *)request
{
    if (!request)
        return NO;

    if (request.finished || request.cancelled)
        return NO;

    // AVURLAsset's resource loader requires knowing the expected content size
    // to load sucessfully. That requires either having the complete data for
    // the resource, or knowing the expected content size. 
    if (!_complete && !_expectedContentSize)
        return NO;

    if (auto dataRequest = request.dataRequest) {
        if (dataRequest.requestedOffset > static_cast<long long>(_data.get().length))
            return NO;
    }

    return YES;
}

- (void)enqueueRequest:(AVAssetResourceLoadingRequest *)loadingRequest
{
    ASSERT(!_requests.contains(loadingRequest));
    _requests.append(loadingRequest);
}

- (void)fulfillPendingRequests
{
    for (auto& request : _requests) {
        if ([self canFulfillRequest:request.get()])
            [self fulfillRequest:request.get()];
    }

    _requests.removeAllMatching([] (auto& request) {
        return request.get().finished;
    });
}

- (void)fulfillRequest:(AVAssetResourceLoadingRequest *)request
{
    if (auto infoRequest = request.contentInformationRequest) {
        infoRequest.contentType = _parent->uti();
        infoRequest.byteRangeAccessSupported = YES;
        infoRequest.contentLength = _complete ? _data.get().length : _expectedContentSize;
    }

    if (auto dataRequest = request.dataRequest) {
        long long availableLength = _data.get().length - dataRequest.requestedOffset;
        if (availableLength <= 0)
            return;

        long long requestedLength;
        if (dataRequest.requestsAllDataToEndOfResource)
            requestedLength = availableLength;
        else
            requestedLength = std::min<long long>(availableLength, dataRequest.requestedLength);

        auto range = NSMakeRange(static_cast<NSUInteger>(dataRequest.requestedOffset), static_cast<NSUInteger>(requestedLength));
        NSData* requestedData = [_data subdataWithRange:range];
        if (!requestedData)
            return;

        [dataRequest respondWithData:requestedData];

        if (dataRequest.requestsAllDataToEndOfResource) {
            if (!_complete)
                return;
        } else if (dataRequest.requestedOffset + dataRequest.requestedLength > dataRequest.currentOffset)
            return;
    }

    [request finishLoading];
}

- (BOOL)resourceLoader:(AVAssetResourceLoader *)resourceLoader shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest
{
    LockHolder holder { _dataLock };

    UNUSED_PARAM(resourceLoader);

    if ([self canFulfillRequest:loadingRequest]) {
        [self fulfillRequest:loadingRequest];
        if (loadingRequest.finished)
            return NO;
    }

    [self enqueueRequest:loadingRequest];
    return YES;
}

- (void)resourceLoader:(AVAssetResourceLoader *)resourceLoader didCancelLoadingRequest:(AVAssetResourceLoadingRequest *)loadingRequest
{
    LockHolder holder { _dataLock };

    UNUSED_PARAM(resourceLoader);
    _requests.removeAll(loadingRequest);
}
@end

namespace WebCore {

#pragma mark - Static Methods

static NSURL *customSchemeURL()
{
    static NeverDestroyed<RetainPtr<NSURL>> url;
    if (!url.get())
        url.get() = adoptNS([[NSURL alloc] initWithString:@"custom-imagedecoderavfobjc://resource"]);

    return url.get().get();
}

static NSDictionary *imageDecoderAssetOptions()
{
    static NeverDestroyed<RetainPtr<NSDictionary>> options;
    if (!options.get()) {
        options.get() = @{
            AVURLAssetReferenceRestrictionsKey: @(AVAssetReferenceRestrictionForbidAll),
            AVURLAssetUsesNoPersistentCacheKey: @YES,
        };
    }

    return options.get().get();
}

static ImageDecoderAVFObjC::RotationProperties transformToRotationProperties(AffineTransform inTransform)
{
    ImageDecoderAVFObjC::RotationProperties rotation;
    if (inTransform.isIdentity())
        return rotation;

    AffineTransform::DecomposedType decomposed { };
    if (!inTransform.decompose(decomposed))
        return rotation;

    rotation.flipY = WTF::areEssentiallyEqual(decomposed.scaleX, -1.);
    rotation.flipX = WTF::areEssentiallyEqual(decomposed.scaleY, -1.);
    auto degrees = rad2deg(decomposed.angle);
    while (degrees < 0)
        degrees += 360;

    // Only support rotation in multiples of 90º:
    if (WTF::areEssentiallyEqual(fmod(degrees, 90.), 0.))
        rotation.angle = clampToUnsigned(degrees);

    return rotation;
}

struct ImageDecoderAVFObjC::SampleData {
    Seconds duration { 0 };
    bool hasAlpha { false };
    IntSize frameSize;
    RetainPtr<CMSampleBufferRef> sample;
    RetainPtr<CGImageRef> image;
    MediaTime decodeTime;
    MediaTime presentationTime;
};

#pragma mark - ImageDecoderAVFObjC

RefPtr<ImageDecoderAVFObjC> ImageDecoderAVFObjC::create(SharedBuffer& data, const String& mimeType, AlphaOption alphaOption, GammaAndColorProfileOption gammaAndColorProfileOption)
{
    // AVFoundation may not be available at runtime.
    if (!getAVURLAssetClass())
        return nullptr;

    if (!canLoad_VideoToolbox_VTCreateCGImageFromCVPixelBuffer())
        return nullptr;

    return adoptRef(*new ImageDecoderAVFObjC(data, mimeType, alphaOption, gammaAndColorProfileOption));
}

ImageDecoderAVFObjC::ImageDecoderAVFObjC(SharedBuffer& data, const String& mimeType, AlphaOption, GammaAndColorProfileOption)
    : ImageDecoder()
    , m_mimeType(mimeType)
    , m_uti(WebCore::UTIFromMIMEType(mimeType))
    , m_asset(adoptNS([allocAVURLAssetInstance() initWithURL:customSchemeURL() options:imageDecoderAssetOptions()]))
    , m_loader(adoptNS([[WebCoreSharedBufferResourceLoaderDelegate alloc] initWithParent:this]))
    , m_decompressionSession(WebCoreDecompressionSession::createRGB())
{
    [m_loader updateData:data.createNSData().get() complete:NO];

    [m_asset.get().resourceLoader setDelegate:m_loader.get() queue:dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0)];
    [m_asset loadValuesAsynchronouslyForKeys:@[@"tracks"] completionHandler:[protectedThis = makeRefPtr(this)] () mutable {
        callOnMainThread([protectedThis = WTFMove(protectedThis)] {
            protectedThis->setTrack(protectedThis->firstEnabledTrack());
        });
    }];
}

ImageDecoderAVFObjC::~ImageDecoderAVFObjC() = default;

bool ImageDecoderAVFObjC::supportsMediaType(MediaType type)
{
    if (type == MediaType::Video)
        return getAVURLAssetClass() && canLoad_VideoToolbox_VTCreateCGImageFromCVPixelBuffer();
    return false;
}

bool ImageDecoderAVFObjC::supportsContentType(const ContentType& type)
{
    if (getAVURLAssetClass() && canLoad_VideoToolbox_VTCreateCGImageFromCVPixelBuffer())
        return AVFoundationMIMETypeCache::singleton().types().contains(type.containerType());
    return false;
}

bool ImageDecoderAVFObjC::canDecodeType(const String& mimeType)
{
    if (!supportsMediaType(MediaType::Video))
        return nullptr;

    return [getAVURLAssetClass() isPlayableExtendedMIMEType:mimeType];
}

AVAssetTrack *ImageDecoderAVFObjC::firstEnabledTrack()
{
    NSArray<AVAssetTrack *> *videoTracks = [m_asset tracksWithMediaCharacteristic:AVMediaCharacteristicVisual];
    NSUInteger firstEnabledIndex = [videoTracks indexOfObjectPassingTest:^(AVAssetTrack *track, NSUInteger, BOOL*) {
        return track.enabled;
    }];

    if (firstEnabledIndex == NSNotFound) {
        LOG(Images, "ImageDecoderAVFObjC::firstEnabledTrack(%p) - asset has no enabled video tracks", this);
        return nil;
    }

    return [videoTracks objectAtIndex:firstEnabledIndex];
}

void ImageDecoderAVFObjC::readSampleMetadata()
{
    if (!m_sampleData.isEmpty())
        return;

    // NOTE: there is no API to return the number of samples in the sample table. Instead,
    // simply increment the sample in decode order by an arbitrarily large number.
    RetainPtr<AVSampleCursor> cursor = [m_track makeSampleCursorAtFirstSampleInDecodeOrder];
    int64_t sampleCount = 0;
    if (cursor)
        sampleCount = 1 + [cursor stepInDecodeOrderByCount:std::numeric_limits<int32_t>::max()];

    // NOTE: there is no API to return the first sample cursor in presentation order. Instead,
    // simply decrement sample in presentation order by an arbitrarily large number.
    [cursor stepInPresentationOrderByCount:std::numeric_limits<int32_t>::min()];

    ASSERT(sampleCount >= 0);
    m_sampleData.resize(static_cast<size_t>(sampleCount));

    if (!m_generator)
        m_generator = adoptNS([allocAVSampleBufferGeneratorInstance() initWithAsset:m_asset.get() timebase:nil]);

    for (size_t index = 0; index < static_cast<size_t>(sampleCount); ++index) {
        auto& sampleData = m_sampleData[index];
        sampleData.duration = Seconds(PAL::CMTimeGetSeconds([cursor currentSampleDuration]));
        sampleData.decodeTime = PAL::toMediaTime([cursor decodeTimeStamp]);
        sampleData.presentationTime = PAL::toMediaTime([cursor presentationTimeStamp]);
        auto request = adoptNS([allocAVSampleBufferRequestInstance() initWithStartCursor:cursor.get()]);
        sampleData.sample = adoptCF([m_generator createSampleBufferForRequest:request.get()]);
        m_presentationTimeToIndex.insert(std::make_pair(sampleData.presentationTime, index));
        [cursor stepInPresentationOrderByCount:1];
    }
}

void ImageDecoderAVFObjC::readTrackMetadata()
{
    if (!m_rotation)
        m_rotation = transformToRotationProperties(CGAffineTransformConcat(m_asset.get().preferredTransform, m_track.get().preferredTransform));

    if (!m_size) {
        auto size = FloatSize(m_track.get().naturalSize);
        auto angle = m_rotation.value().angle;
        if (angle == 90 || angle == 270)
            size = size.transposedSize();

        m_size = expandedIntSize(size);
    }
}

bool ImageDecoderAVFObjC::storeSampleBuffer(CMSampleBufferRef sampleBuffer)
{
    auto pixelBuffer = m_decompressionSession->decodeSampleSync(sampleBuffer);
    if (!pixelBuffer) {
        LOG(Images, "ImageDecoderAVFObjC::storeSampleBuffer(%p) - could not decode sampleBuffer", this);
        return false;
    }

    auto presentationTime = PAL::toMediaTime(PAL::CMSampleBufferGetPresentationTimeStamp(sampleBuffer));
    auto indexIter = m_presentationTimeToIndex.find(presentationTime);

    if (m_rotation && !m_rotation.value().isIdentity()) {
        auto& rotation = m_rotation.value();
        if (!m_rotationSession) {
            VTImageRotationSessionRef rawRotationSession = nullptr;
            VTImageRotationSessionCreate(kCFAllocatorDefault, rotation.angle, &rawRotationSession);
            m_rotationSession = rawRotationSession;
            VTImageRotationSessionSetProperty(m_rotationSession.get(), kVTImageRotationPropertyKey_EnableHighSpeedTransfer, kCFBooleanTrue);

            if (rotation.flipY)
                VTImageRotationSessionSetProperty(m_rotationSession.get(), kVTImageRotationPropertyKey_FlipVerticalOrientation, kCFBooleanTrue);
            if (rotation.flipX)
                VTImageRotationSessionSetProperty(m_rotationSession.get(), kVTImageRotationPropertyKey_FlipHorizontalOrientation, kCFBooleanTrue);
        }

        if (!m_rotationPool) {
            auto pixelAttributes = (CFDictionaryRef)@{
                (NSString *)kCVPixelBufferWidthKey: @(m_size.value().width()),
                (NSString *)kCVPixelBufferHeightKey: @(m_size.value().height()),
                (NSString *)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
                (NSString *)kCVPixelBufferCGImageCompatibilityKey: @YES,
            };
            CVPixelBufferPoolRef rawPool = nullptr;
            CVPixelBufferPoolCreate(kCFAllocatorDefault, nullptr, pixelAttributes, &rawPool);
            m_rotationPool = adoptCF(rawPool);
        }

        CVPixelBufferRef rawRotatedBuffer = nullptr;
        CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, m_rotationPool.get(), &rawRotatedBuffer);
        auto status = VTImageRotationSessionTransferImage(m_rotationSession.get(), pixelBuffer.get(), rawRotatedBuffer);
        if (status == noErr)
            pixelBuffer = adoptCF(rawRotatedBuffer);
    }

    CGImageRef rawImage = nullptr;
    if (noErr != VTCreateCGImageFromCVPixelBuffer(pixelBuffer.get(), nullptr, &rawImage)) {
        LOG(Images, "ImageDecoderAVFObjC::storeSampleBuffer(%p) - could not create CGImage from pixelBuffer", this);
        return false;
    }

    ASSERT(indexIter->second < m_sampleData.size());
    auto& sampleData = m_sampleData[indexIter->second];
    sampleData.image = adoptCF(rawImage);
    sampleData.sample = nullptr;

    auto alphaInfo = CGImageGetAlphaInfo(rawImage);
    sampleData.hasAlpha = (alphaInfo != kCGImageAlphaNone && alphaInfo != kCGImageAlphaNoneSkipLast && alphaInfo != kCGImageAlphaNoneSkipFirst);

    return true;
}

void ImageDecoderAVFObjC::advanceCursor()
{
    if (![m_cursor stepInDecodeOrderByCount:1])
        m_cursor = [m_track makeSampleCursorAtFirstSampleInDecodeOrder];
}

void ImageDecoderAVFObjC::setTrack(AVAssetTrack *track)
{
    if (m_track == track)
        return;
    m_track = track;

    LockHolder holder { m_sampleGeneratorLock };
    m_sampleData.clear();
    m_size.reset();
    m_rotation.reset();
    m_cursor = nullptr;
    m_generator = nullptr;
    m_rotationSession = nullptr;

    [track loadValuesAsynchronouslyForKeys:@[@"naturalSize", @"preferredTransform"] completionHandler:[protectedThis = makeRefPtr(this)] () mutable {
        callOnMainThread([protectedThis = WTFMove(protectedThis)] {
            protectedThis->readTrackMetadata();
            protectedThis->readSampleMetadata();
        });
    }];
}

EncodedDataStatus ImageDecoderAVFObjC::encodedDataStatus() const
{
    if (m_sampleData.isEmpty())
        return EncodedDataStatus::Unknown;
    return EncodedDataStatus::Complete;
}

IntSize ImageDecoderAVFObjC::size() const
{
    if (m_size)
        return m_size.value();
    return IntSize();
}

size_t ImageDecoderAVFObjC::frameCount() const
{
    return m_sampleData.size();
}

RepetitionCount ImageDecoderAVFObjC::repetitionCount() const
{
    // In the absence of instructions to the contrary, assume all media formats repeat infinitely.
    // FIXME: Future media formats may embed repeat count information, and when that is available
    // through AVAsset, account for it here.
    return RepetitionCountInfinite;
}

String ImageDecoderAVFObjC::uti() const
{
    return m_uti;
}

String ImageDecoderAVFObjC::filenameExtension() const
{
    return MIMETypeRegistry::getPreferredExtensionForMIMEType(m_mimeType);
}

IntSize ImageDecoderAVFObjC::frameSizeAtIndex(size_t, SubsamplingLevel) const
{
    return size();
}

bool ImageDecoderAVFObjC::frameIsCompleteAtIndex(size_t index) const
{
    if (index >= m_sampleData.size())
        return false;

    auto sampleData = m_sampleData[index];
    if (!sampleData.sample)
        return false;

    return PAL::CMSampleBufferDataIsReady(sampleData.sample.get());
}

ImageOrientation ImageDecoderAVFObjC::frameOrientationAtIndex(size_t) const
{
    return ImageOrientation();
}

Seconds ImageDecoderAVFObjC::frameDurationAtIndex(size_t index) const
{
    if (index < m_sampleData.size())
        return m_sampleData[index].duration;
    return { };
}

bool ImageDecoderAVFObjC::frameHasAlphaAtIndex(size_t index) const
{
    if (index < m_sampleData.size())
        return m_sampleData[index].hasAlpha;
    return false;
}

bool ImageDecoderAVFObjC::frameAllowSubsamplingAtIndex(size_t index) const
{
    return index <= m_sampleData.size();
}

unsigned ImageDecoderAVFObjC::frameBytesAtIndex(size_t index, SubsamplingLevel subsamplingLevel) const
{
    if (!frameIsCompleteAtIndex(index))
        return 0;

    IntSize frameSize = frameSizeAtIndex(index, subsamplingLevel);
    return (frameSize.area() * 4).unsafeGet();
}

NativeImagePtr ImageDecoderAVFObjC::createFrameImageAtIndex(size_t index, SubsamplingLevel, const DecodingOptions&)
{
    LockHolder holder { m_sampleGeneratorLock };

    if (index >= m_sampleData.size())
        return nullptr;

    auto& sampleData = m_sampleData[index];
    if (sampleData.image)
        return sampleData.image;

    if (!m_cursor)
        m_cursor = [m_track makeSampleCursorAtFirstSampleInDecodeOrder];

    auto frameCursor = [m_track makeSampleCursorWithPresentationTimeStamp:PAL::toCMTime(sampleData.presentationTime)];
    if ([frameCursor comparePositionInDecodeOrderWithPositionOfCursor:m_cursor.get()] == NSOrderedAscending)  {
        // Rewind cursor to the last sync sample to begin decoding
        m_cursor = adoptNS([frameCursor copy]);
        do {
            if ([m_cursor currentSampleSyncInfo].sampleIsFullSync)
                break;
        } while ([m_cursor stepInDecodeOrderByCount:-1] == -1);

    }

    if (!m_generator)
        m_generator = adoptNS([allocAVSampleBufferGeneratorInstance() initWithAsset:m_asset.get() timebase:nil]);

    RetainPtr<CGImageRef> image;
    while (true) {
        if ([frameCursor comparePositionInDecodeOrderWithPositionOfCursor:m_cursor.get()] == NSOrderedAscending)
            return nullptr;

        auto presentationTime = PAL::toMediaTime(m_cursor.get().presentationTimeStamp);
        auto indexIter = m_presentationTimeToIndex.find(presentationTime);

        if (indexIter == m_presentationTimeToIndex.end())
            break;

        auto& cursorSampleData = m_sampleData[indexIter->second];

        if (!cursorSampleData.sample) {
            auto request = adoptNS([allocAVSampleBufferRequestInstance() initWithStartCursor:m_cursor.get()]);
            cursorSampleData.sample = adoptCF([m_generator createSampleBufferForRequest:request.get()]);
        }

        if (!cursorSampleData.sample)
            break;

        if (!storeSampleBuffer(cursorSampleData.sample.get()))
            break;

        advanceCursor();
        if (sampleData.image)
            return sampleData.image;
    }

    advanceCursor();
    return nullptr;
}

void ImageDecoderAVFObjC::setExpectedContentSize(long long expectedContentSize)
{
    m_loader.get().expectedContentSize = expectedContentSize;
}

void ImageDecoderAVFObjC::setData(SharedBuffer& data, bool allDataReceived)
{
    [m_loader updateData:data.createNSData().get() complete:allDataReceived];

    if (allDataReceived) {
        m_isAllDataReceived = true;

        if (!m_track)
            setTrack(firstEnabledTrack());

        readTrackMetadata();
        readSampleMetadata();
    }
}

void ImageDecoderAVFObjC::clearFrameBufferCache(size_t index)
{
    for (size_t i = 0; i < index; ++i)
        m_sampleData[i].image = nullptr;
}

}

#endif
