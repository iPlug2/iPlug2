#import "GenericUI.h"

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

static void *kvoParameterValue = &kvoParameterValue;

@implementation ParameterView

- (instancetype)initWithParameter:(AUParameter *)parameter
{
  if (self = [super init])
  {
    self.axis = UILayoutConstraintAxisHorizontal;
    self.alignment = UIStackViewAlignmentCenter;
    self.spacing = 2;
    
    _parameter = parameter;
    
    _nameLabel = [self pv_labelWithText:[parameter.displayName stringByAppendingString:@":"] alignment:NSTextAlignmentRight];
    _minValueLabel = [self pv_labelWithText:[NSString stringWithFormat:@"%.0f", parameter.minValue] alignment:NSTextAlignmentRight];
    _maxValueLabel = [self pv_labelWithText:[NSString stringWithFormat:@"%.0f", parameter.maxValue] alignment:NSTextAlignmentLeft];
    
    _valueSlider = [[UISlider alloc] init];
    _valueSlider.translatesAutoresizingMaskIntoConstraints = NO;
    _valueSlider.minimumValue = parameter.minValue;
    _valueSlider.maximumValue = parameter.maxValue;
    
    _valueTextField = [[UITextField alloc] init];
    _valueTextField.translatesAutoresizingMaskIntoConstraints = NO;
    _valueTextField.borderStyle = UITextBorderStyleRoundedRect;
    _valueTextField.text = [self stringWithValue:parameter.minValue];
    CGFloat width = [_valueTextField systemLayoutSizeFittingSize:CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX)].width;
    _valueTextField.text = [self stringWithValue:parameter.maxValue];
    width = MAX(width, [_valueTextField systemLayoutSizeFittingSize:CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX)].width);
    [_valueTextField.widthAnchor constraintGreaterThanOrEqualToConstant:width].active = YES;
    
    [self addArrangedSubview:_nameLabel];
    [self addArrangedSubview:_minValueLabel];
    [self addArrangedSubview:_valueSlider];
    [self addArrangedSubview:_maxValueLabel];
    [self addArrangedSubview:_valueTextField];
    
    [_parameter addObserver:self forKeyPath:@"value" options:0 context:kvoParameterValue];
    [_valueSlider addTarget:self action:@selector(sliderValueChanged:) forControlEvents:UIControlEventValueChanged];
    [self updateViews];
  }
  return self;
}

- (UILabel *) pv_labelWithText:(NSString *)text alignment:(NSTextAlignment)alignment
{
  UILabel* label = [[UILabel alloc] init];
  label.translatesAutoresizingMaskIntoConstraints = NO;
  label.text = text;
  label.textAlignment = alignment;
  [label setContentCompressionResistancePriority:UILayoutPriorityRequired - 1 forAxis:UILayoutConstraintAxisHorizontal];
  [label setContentHuggingPriority:UILayoutPriorityDefaultHigh forAxis:UILayoutConstraintAxisHorizontal];
  return label;
}

- (void)constrainColumnsToReferenceView:(ParameterView *)referenceView {
  [NSLayoutConstraint activateConstraints:@[
                                            [_nameLabel.widthAnchor constraintEqualToAnchor:referenceView.nameLabel.widthAnchor],
                                            [_minValueLabel.widthAnchor constraintEqualToAnchor:referenceView.minValueLabel.widthAnchor],
                                            [_valueSlider.widthAnchor constraintEqualToAnchor:referenceView.valueSlider.widthAnchor],
                                            [_maxValueLabel.widthAnchor constraintEqualToAnchor:referenceView.maxValueLabel.widthAnchor],
                                            [_valueTextField.widthAnchor constraintEqualToAnchor:referenceView.valueTextField.widthAnchor],
                                            ]];
}

- (void)sliderValueChanged:(UISlider *)slider
{
  _parameter.value = slider.value;
}

- (void)updateViews
{
  _valueSlider.value = _parameter.value;
  _valueTextField.text = [self stringWithValue:_parameter.value];
}

- (NSString *)stringWithValue:(double)value
{
  return [NSString stringWithFormat:@"%.4f", value];
}

- (void)dealloc {
  [_parameter removeObserver:self forKeyPath:@"value" context:kvoParameterValue];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context
{
  if (context == kvoParameterValue)
  {
    [self updateViews];
  }
  else
  {
    [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
  }
}

@end

@implementation GenericUI
- (instancetype) initWithAUPlugin:(AUAudioUnit*) plugin
{
  if (self = [super init])
  {
    UIStackView* rootStack = [[UIStackView alloc] init];
    rootStack.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:rootStack];
    [NSLayoutConstraint activateConstraints:@[
                                              [rootStack.leadingAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.leadingAnchor constant:8],
                                              [rootStack.topAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.topAnchor constant:8],
                                              [rootStack.trailingAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.trailingAnchor constant:-8],
                                              ]];
    rootStack.axis = UILayoutConstraintAxisVertical;
    rootStack.spacing = 4;
    rootStack.alignment = UIStackViewAlignmentFill;
    
    AUParameterTree* tree = [plugin parameterTree];
    
    ParameterView* firstParameterView;
    for (AUParameter* p in tree.allParameters) {
      ParameterView *pv = [[ParameterView alloc] initWithParameter:p];
      [rootStack addArrangedSubview:pv];
      if (firstParameterView == nil) {
        firstParameterView = pv;
      } else {
        [pv constrainColumnsToReferenceView:firstParameterView];
      }
    }
  }
  
  return self;
}
@end
