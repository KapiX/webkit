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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#import <Availability.h>
#import <wtf/Assertions.h>
#import <wtf/RetainPtr.h>

#define USE_SECURE_ARCHIVER_API ((PLATFORM(MAC) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101302 && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110200) || (PLATFORM(WATCHOS) && __WATCH_OS_VERSION_MIN_REQUIRED >= 40200) || (PLATFORM(TVOS) && __TV_OS_VERSION_MIN_REQUIRED >= 110200))

#if USE(SECURE_ARCHIVER_API)
#if USE(APPLE_INTERNAL_SDK)
#import <Foundation/NSKeyedArchiver_Private.h>
#else
#import <Foundation/NSCoder.h>
#import <Foundation/NSKeyedArchiver.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSKeyedArchiver (NSKeyedArchiverSecureCodingInitializers)
- (instancetype)initRequiringSecureCoding:(BOOL)requiresSecureCoding API_AVAILABLE(macos(10.13), ios(11.0));
+ (nullable NSData *)archivedDataWithRootObject:(id)object requiringSecureCoding:(BOOL)requiresSecureCoding error:(NSError **)error API_AVAILABLE(macos(10.13), ios(11.0));
@end

@interface NSKeyedUnarchiver (NSKeyedUnarchiverSecureCodingInitializer)
- (nullable instancetype)initForReadingFromData:(NSData *)data error:(NSError **)error API_AVAILABLE(macos(10.13), ios(11.0));
+ (nullable id)unarchivedObjectOfClass:(Class)cls fromData:(NSData *)data error:(NSError **)error API_AVAILABLE(macos(10.13), ios(11.0));
+ (nullable id)unarchivedObjectOfClasses:(NSSet<Class> *)classes fromData:(NSData *)data error:(NSError **)error API_AVAILABLE(macos(10.13), ios(11.0));
@end

NS_ASSUME_NONNULL_END

#endif /* USE(APPLE_INTERNAL_SDK) */
#endif /* USE(SECURE_ARCHIVER_API) */

inline NSData *_Nullable securelyArchivedDataWithRootObject(id _Nonnull object)
{
#if USE(SECURE_ARCHIVER_API)
    NSError *error;
    NSData *data = [NSKeyedArchiver archivedDataWithRootObject:object requiringSecureCoding:YES error:&error];
    if (!data)
        LOG_ERROR("Unable to archive data: %@", error);
    return data;
#else
    return [NSKeyedArchiver archivedDataWithRootObject:object];
#endif
}

inline NSData *_Nullable insecurelyArchivedDataWithRootObject(id _Nonnull object)
{
#if USE(SECURE_ARCHIVER_API)
    NSError *error;
    NSData *data = [NSKeyedArchiver archivedDataWithRootObject:object requiringSecureCoding:NO error:&error];
    if (!data)
        LOG_ERROR("Unable to archive data: %@", error);
    return data;
#else
    return [NSKeyedArchiver archivedDataWithRootObject:object];
#endif
}

inline id _Nullable securelyUnarchiveObjectOfClassFromData(Class _Nonnull cls, NSData * _Nonnull data)
{
#if USE(SECURE_ARCHIVER_API)
    NSError *error;
    id value = [NSKeyedUnarchiver unarchivedObjectOfClass:cls fromData:data error:&error];
    if (!data)
        LOG_ERROR("Unable to unarchive data: %@", error);
    return value;
#else
    UNUSED_PARAM(cls);
    return [NSKeyedUnarchiver unarchiveObjectWithData:data];
#endif
}

inline id _Nullable insecurelyUnarchiveObjectOfClassFromData(NSData * _Nonnull data)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    return [NSKeyedUnarchiver unarchiveObjectWithData:data];
#pragma clang diagnostic pop
}

inline RetainPtr<NSKeyedArchiver> secureArchiver()
{
#if USE(SECURE_ARCHIVER_API)
    NSKeyedArchiver *archiver = [[NSKeyedArchiver alloc] initRequiringSecureCoding:YES];
#else
    NSKeyedArchiver *archiver = [[NSKeyedArchiver alloc] init];
    [archiver setRequiresSecureCoding:YES];
#endif
    return adoptNS(archiver);
}

inline RetainPtr<NSKeyedUnarchiver> secureUnarchiverFromData(NSData *_Nonnull data)
{
#if USE(SECURE_ARCHIVER_API)
    NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingFromData:data error:nil];
    unarchiver.decodingFailurePolicy = NSDecodingFailurePolicyRaiseException;
#else
    NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData:data];
    [unarchiver setRequiresSecureCoding:YES];
#endif
    return adoptNS(unarchiver);
}

