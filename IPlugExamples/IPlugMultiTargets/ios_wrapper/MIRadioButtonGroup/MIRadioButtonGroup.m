// hacked by oli larkin


/*

 Copyright (c) 2010, Mobisoft Infotech
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are
 permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this list of
 conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 Neither the name of Mobisoft Infotech nor the names of its contributors may be used to
 endorse or promote products derived from this software without specific prior written permission.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
 OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 OF SUCH DAMAGE.

 */

#import "MIRadioButtonGroup.h"

@implementation MIRadioButtonGroup
@synthesize radioButtons;

- (id)initWithFrame:(CGRect)frame andOptions:(NSArray *)options andColumns:(int)columns
{

  selectedIdx = 0;

  NSMutableArray *arrTemp =[[NSMutableArray alloc]init];
  self.radioButtons =arrTemp;
  [arrTemp release];
  if (self = [super initWithFrame:frame])
  {
    // Initialization code
    int framex =0;
    framex= frame.size.width/columns;
    int framey = 0;
    framey =frame.size.height/([options count]/(columns));
    int rem =[options count]%columns;
    if(rem !=0)
    {
      framey =frame.size.height/(([options count]/columns)+1);
    }
    int k = 0;
    for(int i=0; i<([options count]/columns); i++)
    {
      for(int j=0; j<columns; j++)
      {
        int x = framex*0.25;
        int y = framey*0.25;
        UIButton *btTemp = [[UIButton alloc]initWithFrame:CGRectMake(framex*j+x, framey*i+y, framex/2+x, framey/2+y)];
        [btTemp addTarget:self action:@selector(radioButtonClicked:) forControlEvents:UIControlEventTouchUpInside];
                btTemp.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
        [btTemp setImage:[UIImage imageNamed:@"radio-off.png"] forState:UIControlStateNormal];
        [btTemp setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
                btTemp.titleLabel.font =[UIFont systemFontOfSize:14.f];
        [btTemp setTitle:[options objectAtIndex:k] forState:UIControlStateNormal];
        [self.radioButtons addObject:btTemp];
        [self addSubview:btTemp];
        [btTemp release];
        k++;

      }
    }

    for(int j=0; j<rem; j++)
    {
      int x = framex*0.25;
      int y = framey*0.25;
      UIButton *btTemp = [[UIButton alloc]initWithFrame:CGRectMake(framex*j+x, framey*([options count]/columns), framex/2+x, framey/2+y)];
      [btTemp addTarget:self action:@selector(radioButtonClicked:) forControlEvents:UIControlEventTouchUpInside];
      btTemp.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
      [btTemp setImage:[UIImage imageNamed:@"radio-off.png"] forState:UIControlStateNormal];
      [btTemp setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
      btTemp.titleLabel.font =[UIFont systemFontOfSize:14.f];
      [btTemp setTitle:[options objectAtIndex:k] forState:UIControlStateNormal];
      [self.radioButtons addObject:btTemp];
      [self addSubview:btTemp];
      [btTemp release];
      k++;
    }

  }
  return self;
}

- (void)dealloc
{
  [radioButtons release];
  [super dealloc];
}

-(IBAction) radioButtonClicked:(UIButton *) sender
{
  for(int i=0; i<[self.radioButtons count]; i++)
  {
    [[self.radioButtons objectAtIndex:i] setImage:[UIImage imageNamed:@"radio-off.png"] forState:UIControlStateNormal];

    if ([self.radioButtons objectAtIndex:i] == sender)
    {
      selectedIdx = i;
    }
  }

  [sender setImage:[UIImage imageNamed:@"radio-on.png"] forState:UIControlStateNormal];
}

-(void) removeButtonAtIndex:(int)index
{
  [[self.radioButtons objectAtIndex:index] removeFromSuperview];
}

-(void) setSelected:(int) index
{
  for(int i=0; i<[self.radioButtons count]; i++)
  {
    [[self.radioButtons objectAtIndex:i] setImage:[UIImage imageNamed:@"radio-off.png"] forState:UIControlStateNormal];
  }
  [[self.radioButtons objectAtIndex:index] setImage:[UIImage imageNamed:@"radio-on.png"] forState:UIControlStateNormal];
  selectedIdx = index;
}

-(void)clearAll
{
  for(int i=0; i<[self.radioButtons count]; i++)
  {
    [[self.radioButtons objectAtIndex:i] setImage:[UIImage imageNamed:@"radio-off.png"] forState:UIControlStateNormal];
  }

}

-(NSInteger)getSelected
{
  return selectedIdx;
}

@end
