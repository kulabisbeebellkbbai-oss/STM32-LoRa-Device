*** Settings ***
Suite Setup                     Setup
Suite Teardown                  Teardown
Test Teardown                   Test Teardown
Resource                        ${RENODEKEYWORDS}

*** Variables ***
${PLATFORM}                     %{RENODE_PLATFORM}
${ELF}                          %{RENODE_ELF}
${UART}                         sysbus.usart1
${BOOT_TIMEOUT}                 10

*** Keywords ***
Create LoRa Device Machine
    Execute Command             mach create
    Execute Command             machine LoadPlatformDescription @${PLATFORM}
    Execute Command             sysbus LoadELF @${ELF}
    Create Terminal Tester      ${UART}  defaultPauseEmulation=${False}

Boot To Command Prompt
    Create LoRa Device Machine
    Start Emulation
    Wait For Line On Uart       LoRaWB LoRa Endpoint boot    timeout=${BOOT_TIMEOUT}
    Wait For Line On Uart       Firmware version: 0.1.0      timeout=${BOOT_TIMEOUT}
    Wait For Line On Uart       firmware_name                 timeout=${BOOT_TIMEOUT}
    Wait For Line On Uart       Hardware drivers are compile-gated; RF transmit remains guarded.    timeout=${BOOT_TIMEOUT}

*** Test Cases ***
Firmware Should Print Boot Banner
    Boot To Command Prompt

Firmware Boot Metadata Regression
    Boot To Command Prompt
