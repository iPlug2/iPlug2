// UIView+PinEdges.m

#import "UIView+PinEdges.h"

@implementation UIView (PinEdges)

- (void)pinToSuperviewEdges {
  if (!self.superview) {
    return;
  }
  
  self.translatesAutoresizingMaskIntoConstraints = NO;
  
  NSLayoutConstraint *top = [self.topAnchor constraintEqualToAnchor:self.superview.topAnchor];
  NSLayoutConstraint *leading = [self.leadingAnchor constraintEqualToAnchor:self.superview.leadingAnchor];
  NSLayoutConstraint *bottom = [self.bottomAnchor constraintEqualToAnchor:self.superview.bottomAnchor];
  NSLayoutConstraint *trailing = [self.trailingAnchor constraintEqualToAnchor:self.superview.trailingAnchor];
  
  [NSLayoutConstraint activateConstraints:@[top, leading, bottom, trailing]];
}

@end
