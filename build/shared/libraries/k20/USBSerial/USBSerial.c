#include <mchck.h>
#include "USBSerial.h"

struct usbd_device cdc_device;
usbd_init_fun_t init_cdc;

struct usb_config_1 {
  struct usb_desc_config_t config;
  struct cdc_function_desc usb_function_0;
};

static const struct usb_config_1 usb_config_1 = {
  .config = {
    .bLength = sizeof(struct usb_desc_config_t),
    .bDescriptorType = USB_DESC_CONFIG,
    .wTotalLength = sizeof(struct usb_config_1),
    .bNumInterfaces = 2,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .one = 1,
    .bMaxPower = 100
  },
  .usb_function_0 = {
    .ctrl_iface = {
      .bLength = sizeof(struct usb_desc_iface_t),
      .bDescriptorType = USB_DESC_IFACE,
      .bInterfaceNumber = 0,
      .bAlternateSetting = 0,
      .bNumEndpoints = 1,
      .bInterfaceClass = USB_DEV_CLASS_CDC,
      .bInterfaceSubClass = USB_DEV_SUBCLASS_CDC_ACM,
      .bInterfaceProtocol = 0,
      .iInterface = 0
    },
    .ctrl_ep = {
      .bLength = sizeof(struct usb_desc_ep_t),
      .bDescriptorType = USB_DESC_EP,
      .ep_num = 1,
      .in = 1,
      .type = USB_EP_INTR,
      .wMaxPacketSize = CDC_NOTICE_SIZE,
      .bInterval = 255
    },
    .data_iface = {
      .bLength = sizeof(struct usb_desc_iface_t),
      .bDescriptorType = USB_DESC_IFACE,
      .bInterfaceNumber = 1,
      .bAlternateSetting = 0,
      .bNumEndpoints = 2,
      .bInterfaceClass = USB_DEV_CLASS_CDC_DCD,
      .bInterfaceSubClass = 0,
      .bInterfaceProtocol = 0,
      .iInterface = 0
    },
    .tx_ep = {
      .bLength = sizeof(struct usb_desc_ep_t),
      .bDescriptorType = USB_DESC_EP,
      .ep_num = 2,
      .in = 1,
      .type = USB_EP_BULK,
      .wMaxPacketSize = CDC_TX_SIZE,
      .bInterval = 255
    },
    .rx_ep = {
      .bLength = sizeof(struct usb_desc_ep_t),
      .bDescriptorType = USB_DESC_EP,
      .ep_num = 1,
      .in = 0,
      .type = USB_EP_BULK,
      .wMaxPacketSize = CDC_RX_SIZE,
      .bInterval = 255
    },
    .cdc_header = {
      .bLength = sizeof(struct cdc_desc_function_header_t),
      .bDescriptorType = {
        .id = USB_DESC_IFACE,
        .type_type = USB_DESC_TYPE_CLASS
      },
      .bDescriptorSubtype = USB_CDC_SUBTYPE_HEADER,
      .bcdCDC = { .maj = 1, .min = 1 }
    },
    .cdc_union = {
      .bLength = sizeof(struct cdc_desc_function_union_t),
      .bDescriptorType = {
        .id = USB_DESC_IFACE,
        .type_type = USB_DESC_TYPE_CLASS
      },
      .bDescriptorSubtype = USB_CDC_SUBTYPE_UNION,
      .bControlInterface = 0,
      .bSubordinateInterface0 = 1
    }
  }
};

static const struct usbd_config usbd_config_1 = {
  .init = init_cdc,
  .desc = &usb_config_1.config,
  .function = { &cdc_function }
};

static const struct usb_desc_dev_t cdc_device_dev_desc = {
  .bLength = sizeof(struct usb_desc_dev_t),
  .bDescriptorType = USB_DESC_DEV,
  .bcdUSB = { .maj = 2 },
  .bDeviceClass = 2, // USB_DEV_CLASS_SEE_IFACE,
  .bDeviceSubClass = 0, //USB_DEV_SUBCLASS_SEE_IFACE,
  .bDeviceProtocol = 0, //USB_DEV_PROTO_SEE_IFACE,
  .bMaxPacketSize0 = EP0_BUFSIZE,
  .idVendor = 0xDE50,
  .idProduct = 1,
  .bcdDevice = { .raw = 0 },
  .iManufacturer = 1,
  .iProduct = 2,
  .iSerialNumber = 3,
  .bNumConfigurations = 1,
};

static const struct usb_desc_string_t * const cdc_device_str_desc[] = {
  USB_DESC_STRING_LANG_ENUS,
  USB_DESC_STRING(u"devsound.se"),
  USB_DESC_STRING(u"DevSound Serial Device"),
  USB_DESC_STRING_SERIALNO,
  NULL
};

struct usbd_device cdc_device = {
  .dev_desc = &cdc_device_dev_desc,
  .string_descs = cdc_device_str_desc,
  .configs = {
    &usbd_config_1,
    NULL
  }
};

static struct cdc_ctx cdc;

static bool volatile usbserial_ready = false, usbserial_txrdy = false;
static char *usbserial_rxdat;
static size_t usbserial_rxlen;

void usbserial_begin() {
  usb_init(&cdc_device);
}

static void usbserial_incoming(uint8_t *data, size_t len) {
  usbserial_rxdat = (char*)data;
  usbserial_rxlen = len;
}

void usbserial_write(void *data, size_t len) {
  if(len == 0 || !usbserial_ready) return;
  while(!usbserial_txrdy);
  usbserial_txrdy = false;
  cdc_write(data, len, &cdc);
}

void usbserial_put(char data) {
  if(!usbserial_ready) return;
  while(!usbserial_txrdy);
  usbserial_txrdy = false;
  cdc_write((const uint8_t*)&data, 1, &cdc);
}

static void usbserial_sent(size_t len) {
  usbserial_txrdy = true;
}

size_t usbserial_available() {
  return usbserial_rxlen;
}

char usbserial_get() {
  if(usbserial_rxlen == 0) return -1;
  char ret = *usbserial_rxdat++;
  if(--usbserial_rxlen == 0) cdc_read_more(&cdc);
  return ret;
}

char usbserial_peek() {
  if(usbserial_rxlen == 0) return -1;
  char ret = *usbserial_rxdat;
  return ret;
}

void init_cdc(int config) {
  cdc_init(usbserial_incoming, usbserial_sent, &cdc);
  usbserial_ready = true;
  usbserial_txrdy = true;
}