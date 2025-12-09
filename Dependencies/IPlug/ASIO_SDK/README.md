# Welcome to ASIO 2.3 SDK Interfaces 

<p align="center">
<img src="Steinberg ASIO Logo Artwork/ASIO-compatible-logo-Steinberg-TM-BW.png" alt="ASIO Logo" width="150"/>

</p>

Here are located all interfaces definitions and examples of **ASIO 2.3** (Audio Stream Input/Outpu). The low-latency, high-performance audio standard.
 
## Table Of Contents

1. [Introduction](#100)
1. [Dual Licensing](#200)
1. [Trademark and Logo Usage](#300)
1. [Frequently Asked Questions (FAQ)](#400)
1. [Additional Resources](#500)
1. [Contributing](#600)
1. [SDK Content](#700)

<div id='100'/>

## Introduction


Audio Stream Input/Output (ASIO) fulfills the requirements of professional audio recording and playback by supporting variable bit depths and sample rates, multi-channel operation, and synchronization. This ensures low latency, high performance, straightforward setup, and stable audio recording for the user.  

The entire system becomes controllable and offers complete and immediate access to the audio system's capabilities. Since its introduction, ASIO has become a supported standard by leading audio hardware manufacturers. 

<div id='200'/>

## Dual Licensing

Developers can choose between two options: 
 - Use of the **proprietary ASIO SDK license**, for use cases that benefit from closed-source licensing.
 - Adopt the [General Public License (GPL) Version 3](https://www.gnu.org/licenses/gpl-3.0.en.html) for full open-source integration.

Both options grant access to the same SDK. 
 
<div id='300'/>

## Trademark and Logo Usage 

### General Principle 

Trademark usage (e.g. "**ASIO**" name or logo) is optional under [GPLv3 License](https://www.gnu.org/licenses/gpl-3.0.en.html), but, if used, must comply with Steinberg's official trademark rules, just like under the proprietary license.

<img src="Steinberg ASIO Logo Artwork/ASIO-compatible-logo-Steinberg-TM-BW.png" alt="ASIO Logo" width="150"/>
 
### Permitted Use (if selected) 

If you choose to use the **ASIO** Compatible Logo or refer to the **ASIO** trademark under either license: 

- Logos must be used in unaltered form.
- Use must follow the official usage guidelines provided in the SDK.
- The trademark/logo must appear only in clear product-related contexts, such as: 
    - Splash screens 
    - About boxes 
    - Product websites 
    - Documentation and manuals 

This ensures legal compliance and brand consistency. 

### Prohibited Use 

- Using “ASIO” (or derivatives like “ASIOi”) in company or product names.
- Applying the logo or trademark to non-ASIO-compatible products.
- Associating the trademark with offensive, obscene, or inappropriate content.

---
### ASIO trademark and/or ASIO Compatible Logo 

(Applies If Trademark or Logo Is Used — **GPLv3** or **Proprietary**) 

If you choose to display the **ASIO** trademark and/or **ASIO Compatible Logo**, you must observe the *Steinberg ASIO Usage Guidelines*. 

*Purpose*: To assist Steinberg in enforcing and protecting the **ASIO** trademark in international jurisdictions.


### Notes for Open-Source Developers 

- The *Steinberg ASIO Usage Guidelines* are included in the SDK.
- Use of branding is not required under [GPLv3 License](https://www.gnu.org/licenses/gpl-3.0.en.html), but if you choose to use it, trademark compliance is mandatory.
- Non-compliance with logo guidelines does not affect your **GPLv3** rights (but may result in trademark violations).

### Best Practice Recommendation 

Although optional, we encourage developers using **ASIO** under [GPLv3 License](https://www.gnu.org/licenses/gpl-3.0.en.html) to follow the *Steinberg ASIO Usage Guidelines*: 

It fosters ecosystem-wide recognition, improves end-user trust, and supports the long-term visibility of the **ASIO** brand.

<div id='400'/>

## Frequently Asked Questions (FAQ)

### Do I need to update my existing ASIO-based products to [GPLv3 License](https://www.gnu.org/licenses/gpl-3.0.en.html)? 
- No. You may continue using the proprietary license for existing products. 

### Can I switch to [GPLv3 License](https://www.gnu.org/licenses/gpl-3.0.en.html)? 
- Yes, if your product is released under [GPLv3 License](https://www.gnu.org/licenses/gpl-3.0.en.html) terms (including source code disclosure). 

### Can I use the ASIO logo under [GPLv3 License](https://www.gnu.org/licenses/gpl-3.0.en.html)? 
- Yes, but only if you comply with the trademark rules outlined above. 

<div id='500'/>

## Additional Resources

- [GPLv3 License Text (GNU.org)](https://www.gnu.org/licenses/gpl-3.0.en.html) 
- [Wikipedia: GNU General Public License Version 3](https://en.wikipedia.org/wiki/GNU_General_Public_License#Version_3)
- [ASIO Forum](https://forums.steinberg.net/c/developer/asio)
- Questions? Contact: [reception@steinberg.de](mailto:reception@steinberg.de)

<div id='600'/>

## Contributing

For bug reports and features requests, please visit the [ASIO/VST3 Developer Forum](https://sdk.steinberg.net).

<div id='700'/>

## SDK Content

| Filename              | Comment                                            |
| --------------------- | -------------------------------------------------- |
|README.md              | this file                                          |
|changes.txt            | contains change information between SDK releases   |
|Steinberg ASIO SDK 2.3.pdf | ASIO SDK 2.3 specification documentation       |
|Steinberg ASIO Licensing Agreement.pdf |  Proprietary Licencing Agreement   |
|Steinberg ASIO Usage Guidelines.pdf |  Usage Guidlines                      |
|LICENSE.txt            | Dual license:  Proprietary Agreement and GPLv3     |
|**common**             | |
|asio.h                 | ASIO C definition                                  |
|iasiodrv.h             | interface definition for the ASIO driver class     |
|asio.cpp               | asio host interface (not used on MacOS)            |
|asiodrvr.h             |                                                    |
|asiodrvr.cpp           | ASIO driver class base definition                  |
|combase.h              | (Windows only)                                     |
|combase.cpp            | COM base definitions (Windows only)                |
|dllentry.cpp           | DLL functions (Windows only)                       |
|register.cpp           | driver self registration functionality             |
|wxdebug.h              |                                                    |
|debugmessage.cpp       | some debugging help                                |
|LICENSE.txt            | license applied to all file in this folder         |
|**host**               | |
|asiodrivers.h          | |
|asiodrivers.cpp        | ASIO driver managment (enumeration and instantiation)|
|ASIOConvertSamples.h   | |
|ASIOConvertSamples.cpp | sample data format conversion class                |
|ginclude.h             | platform specific definitions                      |
|**host/pc**            | |
|asiolist.h             | (Windows only)                                     |
|asiolist.cpp           | instantiates an ASIO driver via the COM model (Windows only)  |
|**host/sample**        | |
|hostsample.cpp         | a simple console app which shows ASIO hosting      |
|hostsample.dsp         | MSVC++ 5.0 project                                 |
|hostsample.vcproj      | Visual Studio 2005 project (32 and 64 bit targets) |
|**driver/asiosample**  | |
|asiosmpl.h             |                                                    |
|asiosmpl.cpp           | ASIO 2.0 sample driver                             |
|wintimer.cpp           | bufferSwitch() wakeup thread (Windows only)        |
|asiosample.def         | Windows DLL module export definition (Windows only) |
|mactimer.cpp           | bufferSwitch() wakeup thread (MacOS only)          |
|macnanosecs.cpp        | Macintosh system reference time (MacOS only)       |
|makesamp.cpp           | Macintosh driver object instantiation (MacOS only) |
|**driver/asiosample/asiosample** | |
|asiosample.dsp         | MSVC++ 5.0 project                                 |
|asiosample.vcproj      | Visual Studio 2005 project (32 and 64 bit targets) |
