setup({ explicit_done: true, explicit_timeout: true });

const validMethod = Object.freeze({
  supportedMethods: "https://apple.com/apple-pay",
  data: {
      version: 2,
      merchantIdentifier: '',
      merchantCapabilities: ['supports3DS'],
      supportedNetworks: ['visa', 'masterCard'],
      countryCode: 'US',
  },
});

const validMethods = Object.freeze([validMethod]);

const validAmount = Object.freeze({
  currency: "USD",
  value: "1.00",
});

const validTotal = Object.freeze({
  label: "Valid total",
  amount: validAmount,
});
const validDetails = {
  total: validTotal,
};

test(() => {
  try {
    new PaymentRequest(validMethods, validDetails);
  } catch (err) {
    done();
    throw err;
  }
}, "Can construct a payment request (smoke test).");

/**
 * Pops up a payment sheet, allowing options to be
 * passed in if particular values are needed.
 *
 * @param PaymentOptions options
 */
async function getPaymentResponse(options, id) {
  const { response } = getPaymentRequestResponse(options, id);
  return response;
}

/**
 * Creates a payment request and response pair.
 * It also shows the payment sheet.
 *
 * @param {PaymentOptions?} options
 * @param {String?} id
 */
async function getPaymentRequestResponse(options, id) {
  const methods = [{
      supportedMethods: "https://apple.com/apple-pay",
      data: {
          version: 2,
          merchantIdentifier: '',
          merchantCapabilities: ['supports3DS'],
          supportedNetworks: ['visa', 'masterCard'],
          countryCode: 'US',
      },
  }];
  const details = {
    id,
    total: {
      label: "Total due",
      amount: { currency: "USD", value: "1.0" },
    },
    shippingOptions: [
      {
        id: "fail1",
        label: "Fail option 1",
        amount: { currency: "USD", value: "5.00" },
        selected: false,
      },
      {
        id: "pass",
        label: "Pass option",
        amount: { currency: "USD", value: "5.00" },
        selected: true,
      },
      {
        id: "fail2",
        label: "Fail option 2",
        amount: { currency: "USD", value: "5.00" },
        selected: false,
      },
    ],
  };
  const request = new PaymentRequest(methods, details, options);
  request.onshippingaddresschange = ev => ev.updateWith(details);
  request.onshippingoptionchange = ev => ev.updateWith(details);
  request.onapplepayvalidatemerchant = ev => {
      ev.complete({});
      internals.mockPaymentCoordinator.acceptPayment();
  };
  const response = await activateThen(() => request.show());
  return { request, response };
}

/**
 * Runs a manual test for payment
 *
 * @param {HTMLButtonElement} button The HTML button.
 * @param {PaymentOptions?} options.
 * @param {Object} expected What property values are expected to pass the test.
 * @param {String?} id And id for the request/response pair.
 */
async function runTest(button, options, expected = {}, id = undefined) {
  button.disabled = true;
  const { request, response } = await getPaymentRequestResponse(options, id);
  await response.complete();
  test(() => {
    assert_idl_attribute(
      response,
      "requestId",
      "Expected requestId to be an IDL attribute."
    );
    assert_equals(response.requestId, request.id, `Expected ids to match`);
    for (const [attribute, value] of Object.entries(expected)) {
      assert_idl_attribute(
        response,
        attribute,
        `Expected ${attribute} to be an IDL attribute.`
      );
      assert_equals(
        response[attribute],
        value,
        `Expected response ${attribute} attribute to be ${value}`
      );
    }
    assert_idl_attribute(response, "details");
    assert_equals(typeof response.details, "object", "Expected an object");
    // Testing that this does not throw:
    response.toJSON();
    if (options && options.requestShipping) {
      assert_equals(
        response.shippingOption,
        "pass",
        "request.shippingOption must be 'pass'"
      );
    } else {
      assert_equals(
        request.shippingOption,
        null,
        "If requestShipping is falsy, request.shippingOption must be null"
      );
      assert_equals(
        response.shippingOption,
        null,
        "request.shippingOption must be null"
      );
    }
  }, button.textContent.trim());
}
