* log in to the [Apple developer portal](https://developer.apple.com/) and manually setup certificates and provisioning profiles for the app

- Certificates, Identifiers & Profiles

    - App group
        - Add a new app group id e.g. group.com.OliLarkin.MySynth and modify Xcode project to use it






    - Identifiers
        - Add main app bundle id e.g. com.OliLarkin.MySynth
            - Enable app groups capability
        - Add AUv3 bundle id e.g. com.OliLarkin.MySynth.AUv3
            - Enable app groups capability

    - Profiles
        - Add "iOS App Development" provisioning profile
