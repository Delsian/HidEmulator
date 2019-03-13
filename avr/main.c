/* Name: main.c
 *
 * Author: Eugene.Krashtan@opensynergy.com
 */

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "usbdrv.h"
#include "oddebug.h"
#include "usbconfig.h"
#include "USI_TWI_Slave.h"

/*
Pin assignment:
PB0 = SDA
PB1 = For programming (MISO)
PB2 = SCL

PB3, PB4 = USB data lines (D-, D+)
*/


typedef uint8_t byte;
 
#define REPORT_SIZE 4
// report frequency set to default of 50hz
#define DEFAULT_REPORT_INTERVAL 12

#ifndef NULL
#define NULL    ((void *)0)
#endif

#ifndef TWI_SLAVE_ADDR
#define TWI_SLAVE_ADDR 0x22
#endif

#define MAGIC 0x5A

/* ------------------------------------------------------------------------- */

static uchar rt_usbHidReportDescriptorSize = 0;
static const uchar *rt_usbHidReportDescriptor = NULL;
static const uchar *rt_usbDeviceDescriptor = NULL;
static uchar rt_usbDeviceDescriptorSize = 0;

/* What was most recently read from the controller */
static unsigned char last_built_report[REPORT_SIZE];

/* What was most recently sent to the host */
static unsigned char last_sent_report[REPORT_SIZE];

uchar        reportBuffer[REPORT_SIZE];

static unsigned char must_report = 0;
static unsigned char idle_rate = DEFAULT_REPORT_INTERVAL; 


char usb_hasCommed = 0;

static uchar	reportReady;

const PROGMEM unsigned char mouse_usbHidReportDescriptor[] = { /* USB report descriptor */
0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x02,        // Usage (Mouse)
0xA1, 0x01,        // Collection (Application)
0x09, 0x01,        //   Usage (Pointer)
0xA1, 0x00,        //   Collection (Physical)
0x05, 0x09,        //     Usage Page (Button)
0x19, 0x01,        //     Usage Minimum (0x01)
0x29, 0x03,        //     Usage Maximum (0x03)
0x15, 0x00,        //     Logical Minimum (0)
0x25, 0x01,        //     Logical Maximum (1)
0x95, 0x03,        //     Report Count (3)
0x75, 0x01,        //     Report Size (1)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x95, 0x01,        //     Report Count (1)
0x75, 0x05,        //     Report Size (5)
0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
0x09, 0x30,        //     Usage (X)
0x09, 0x31,        //     Usage (Y)
0x15, 0x00,        //     Logical Minimum (0)
0x26, 0xFF, 0x0F,  //     Logical Maximum (4095)
0x75, 0x0C,        //     Report Size (12)
0x95, 0x02,        //     Report Count (2)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              //   End Collection
0xC0,              // End Collection
};


#define USBDESCR_DEVICE                 1

const unsigned char usbDescrDevice[] PROGMEM = {        /* USB device descriptor */
        18,                 /* sizeof(usbDescrDevice): length of descriptor in bytes */
        USBDESCR_DEVICE,        /* descriptor type */
        0x01, 0x01, /* USB version supported */
        USB_CFG_DEVICE_CLASS,
        USB_CFG_DEVICE_SUBCLASS,
        0,                  /* protocol */
        8,                  /* max packet size */
        USB_CFG_VENDOR_ID,  /* 2 bytes */
        USB_CFG_DEVICE_ID,  /* 2 bytes */
        USB_CFG_DEVICE_VERSION, /* 2 bytes */
#if USB_CFG_VENDOR_NAME_LEN
        1,                  /* manufacturer string index */
#else
        0,                  /* manufacturer string index */
#endif
#if USB_CFG_DEVICE_NAME_LEN
        2,                  /* product string index */
#else
        0,                  /* product string index */
#endif
#if USB_CFG_SERIAL_NUMBER_LENGTH
        3,                  /* serial number string index */
#else
        0,                  /* serial number string index */
#endif
        1,                  /* number of configurations */
};


void buildReport(unsigned char *reportBuf) {
    if (reportBuf != NULL) {
        memcpy(reportBuf, last_built_report, REPORT_SIZE);
    }
    
    memcpy(last_sent_report, last_built_report, REPORT_SIZE); 
}


void HidBegin() {
    cli();
    usbDeviceDisconnect();
    _delay_ms(200);
    usbDeviceConnect(); 
    
    usbInit();
    
    sei();
}


void HidUpdate() {
    usbPoll();

    // if the report has changed, try force an update anyway
    if (memcmp(last_built_report, last_sent_report, REPORT_SIZE)) {
        must_report = 1;
    }
    
    // if we want to send a report, signal the host computer to ask us for it with a usb 'interrupt'
    if (must_report) {
        if (usbInterruptIsReady()) {
            must_report = 0;
            buildReport(reportBuffer); // put data into reportBuffer
            //clearMove(); // clear deltas
            usbSetInterrupt(reportBuffer, REPORT_SIZE);
        }
    }
}
    
void refresh() {
    HidUpdate();
}

void poll() {
    HidUpdate();
}
   
void HidInit () {
    rt_usbHidReportDescriptor = mouse_usbHidReportDescriptor;
    rt_usbHidReportDescriptorSize = sizeof(mouse_usbHidReportDescriptor);
    rt_usbDeviceDescriptor = usbDescrDevice;
    rt_usbDeviceDescriptorSize = sizeof(usbDescrDevice);
}

// USB_PUBLIC uchar usbFunctionSetup
uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (usbRequest_t *)data;

    usbMsgPtr = reportBuffer;

    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {        /* class request type */
        if (rq->bRequest == USBRQ_HID_GET_REPORT) {  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
            //curGamepad->buildReport(reportBuffer);
            //return curGamepad->report_size;
            return REPORT_SIZE;
        } else if (rq->bRequest == USBRQ_HID_GET_IDLE) {
            usbMsgPtr = &idle_rate;
            return 1;
        } else if (rq->bRequest == USBRQ_HID_SET_IDLE) {
            idle_rate = rq->wValue.bytes[1];
        }
    } else {
        /* no vendor specific requests implemented */
    }
    return 0;
}

uchar   usbFunctionDescriptor(struct usbRequest *rq) {
    if ((rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_STANDARD) {
        return 0;
    }

    if (rq->bRequest == USBRQ_GET_DESCRIPTOR) {
        // USB spec 9.4.3, high byte is descriptor type
        switch (rq->wValue.bytes[1]) {
            case USBDESCR_DEVICE:
                usbMsgPtr = rt_usbDeviceDescriptor;
                return rt_usbDeviceDescriptorSize;
                break;

            case USBDESCR_HID_REPORT:
                usbMsgPtr = rt_usbHidReportDescriptor;
                return rt_usbHidReportDescriptorSize;
                break;
        }
    }

    return 0;
}


/* ------------------------------------------------------------------------- */

static void timerInit(void) {
    TCCR1 = 0x0b;           /* select clock: 16.5M/1k -> overflow rate = 16.5M/256k = 62.94 Hz */
}

static void timerPoll(void)
{
static uchar timerCnt;

    if(TIFR & (1 << TOV1)){
        TIFR = (1 << TOV1); /* clear overflow */
        if(++timerCnt >= idle_rate){       /* ~ 1 second interval */
            timerCnt = 0;
            must_report = 1;
        }
    }
}

static void i2cPoll(void) {
    static unsigned char  cnt = 0;
    unsigned char  x,y,z,b;
    if (usiTwiDataInReceiveBuffer()) {
        unsigned char received = usiTwiReceiveByte();
        if (cnt > 0) // Receive bytes
        {
            cnt++;
            switch(cnt) { 
                case 2:  
                    x=received;
                    break;
                case 3:
                    y=received;
                    break;
                case 4:  
                    z=received;
                    break; 
                case 5:  
                    b=received;
                break; 
            }  
            if (cnt==5) // all done
            {
                last_built_report[0] = b;
                last_built_report[1] = x;
                last_built_report[2] = y;
                last_built_report[3] = z; 
                cnt = 0;  
            }
        }
        if (received == MAGIC && cnt == 0) // Start of packet
        {
            cnt = 1;
        }    
    }
}

/* ------------------------------------------------------------------------- */
/* --------------------------------- main ---------------------------------- */
/* ------------------------------------------------------------------------- */

int __attribute__((noreturn)) main(void)
{
    wdt_enable(WDTO_1S);
    odDebugInit();

    HidInit();

    timerInit();
    
    reportReady = 0;
    HidBegin();

    usiTwiSlaveInit( TWI_SLAVE_ADDR );
    sei();
    for(;;){    /* main event loop */
        wdt_reset();
        timerPoll();
        poll();
        i2cPoll();
    }
}

/* ------------------------------------------------------------------------- */

