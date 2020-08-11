# IPlug Platform Abstraction Layer

### Notes for developers.

Never include any headerfile directly from the underlying folders.
To include headers for a platform implementation, use the 'PLATFORM_HEADER(<file>)' macro.

    #include PLATFORM_HEADER(MyPlatformClass.h)

The included header file should not include any platform specific headers or code.
