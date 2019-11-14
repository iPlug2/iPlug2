#import <UIKit/UIKit.h>
#import <AudioUnit/AudioUnit.h>

@interface ParameterView: UIStackView
@property (nonatomic, strong, readonly, nonnull) AUParameter *parameter;

@property (nonatomic, strong, readonly, nonnull) UILabel *nameLabel;
@property (nonatomic, strong, readonly, nonnull) UILabel *minValueLabel;
@property (nonatomic, strong, readonly, nonnull) UISlider *valueSlider;
@property (nonatomic, strong, readonly, nonnull) UILabel *maxValueLabel;
@property (nonatomic, strong, readonly, nonnull) UITextField *valueTextField;

- (instancetype _Nonnull)initWithParameter:(AUParameter* _Nonnull) parameter;
@end

@interface GenericUI: UIView
- (instancetype _Nonnull) initWithAUPlugin:(AUAudioUnit* _Nonnull) plugin;
@end
