/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/


#pragma once

//---------------------------------------------------------
// Platform configuration

#ifdef _WIN64
	#define PLATFORM_64BIT 1
#else
	#define PLATFORM_64BIT 0
#endif


#define IPLUG_API                __stdcall
#define PLATFORM_LITTLE_ENDIAN   1
#define PLATFORM_CACHE_LINE_SIZE 64
#define PLATFORM_CACHE_ALIGN     __declspec(align(PLATFORM_CACHE_LINE_SIZE))
#define DEBUGBREAK()             __debugbreak()
#define PLATFORM_PTHREADS        0


//---------------------------------------------------------
// There seems to be no reason not to use C++17 in 2020 and onwards
// https://en.cppreference.com/w/cpp/compiler_support#cpp17

#if !defined(_MSVC_LANG) || (_MSVC_LANG < 201703L)
	#error "C++17 conformant compiler required. Use /std:c++17 as compiler option."
#endif

#if !defined(_MSC_VER) || (_MSC_VER < 1924)
	#error "Visual Studio 2019 version 16.4 or higher is required to compile."
#endif


//---------------------------------------------------------
// Setup compiler warnings

// clang-format off
#if defined(_MSC_VER)

	// Mark warnings as errors that needs to be corrected. Don't be lazy.
	// List provided by the good people over at Epic Games.
	#define __WARNINGS_AS_ERRORS \
		4001 4002 4003 4006 4007 4008 4010 4013 4015 4018 4019 4020 4022 4023 4024 4025 4026 4027 4028 4029 \
		4030 4031 4032 4033 4034 4035 4036 4038 4041 4042 4045 4047 4048 4049 4051 4052 4053 4054 4055 4056 \
		4057 4060 4063 4064 4065 4066 4067 4068 4069 4073 4074 4075 4076 4077 4079 4080 4081 4083 4085 4086 \
		4087 4088 4089 4090 4091 4092 4094 4096 4097 4098 4099 4100 4101 4102 4103 4109 4112 4113 4114 4115 \
		4116 4117 4119 4120 4121 4122 4123 4124 4125 4127 4129 4130 4131 4132 4133 4137 4138 4141 4142 4143 \
		4144 4145 4146 4150 4152 4153 4154 4155 4156 4157 4158 4159 4160 4161 4162 4163 4164 4166 4167 4168 \
		4172 4174 4175 4176 4177 4178 4179 4180 4181 4182 4183 4185 4186 4187 4188 4189 4190 4191 4192 4193 \
		4194 4195 4196 4197 4199 4200 4201 4202 4203 4204 4205 4206 4207 4208 4210 4211 4212 4213 4214 4215 \
		4216 4218 4220 4221 4223 4224 4226 4227 4228 4229 4230 4232 4233 4234 4235 4237 4238 4239 4240 4243 \
		4245 4250 4251 4255 4256 4258 4263 4264 4267 4268 4269 4272 4273 4274 4275 4276 4277 4278 4279 4280 \
		4281 4282 4283 4285 4286 4287 4288 4289 4290 4291 4293 4295 4297 4298 4299 4301 4302 4303 4306 4308 \
		4309 4310 4313 4314 4315 4316 4318 4319 4321 4322 4323 4324 4325 4326 4327 4328 4329 4330 4333 4334 \
		4335 4336 4337 4338 4340 4343 4344 4346 4348 4352 4353 4355 4356 4357 4358 4359 4362 4364 4366 4367 \
		4368 4369 4374 4375 4376 4377 4378 4379 4380 4381 4382 4383 4384 4387 4389 4390 4391 4392 4393 4394 \
		4395 4396 4397 4398 4399 4400 4401 4402 4403 4404 4405 4406 4407 4408 4409 4410 4411 4413 4414 4415 \
		4416 4417 4418 4419 4420 4423 4424 4425 4426 4427 4429 4430 4431 4434 4436 4438 4439 4440 4441 4442 \
		4443 4445 4446 4447 4448 4449 4450 4451 4452 4453 4454 4455 4460 4461 4462 4464 4465 4466 4467 4468 \
		4469 4470 4473 4474 4475 4476 4477 4478 4480 4482 4483 4484 4485 4486 4487 4488 4489 4490 4491 4492 \
		4493 4494 4495 4496 4497 4498 4499 4502 4503 4505 4506 4508 4509 4511 4512 4513 4514 4515 4516 4517 \
		4518 4519 4521 4522 4523 4526 4530 4531 4532 4533 4534 4535 4536 4537 4538 4540 4541 4542 4543 4544 \
		4545 4546 4550 4551 4552 4553 4554 4556 4557 4558 4559 4561 4562 4564 4565 4566 4568 4569 4570 4572 \
		4573 4575 4576 4578 4580 4581 4582 4583 4584 4585 4586 4587 4588 4589 4591 4592 4593 4594 4595 4596 \
		4597 4598 4600 4602 4603 4604 4606 4609 4610 4611 4612 4613 4615 4616 4618 4620 4621 4622 4623 4624 \
		4625 4626 4627 4628 4629 4630 4631 4632 4633 4634 4635 4636 4637 4638 4639 4640 4641 4642 4644 4645 \
		4646 4648 4649 4650 4652 4653 4654 4655 4656 4657 4658 4659 4661 4662 4667 4669 4670 4671 4672 4673 \
		4674 4676 4677 4678 4679 4680 4681 4682 4683 4684 4685 4686 4687 4688 4689 4690 4691 4693 4694 4695 \
		4696 4698 4700 4702 4703 4706 4709 4710 4711 4714 4715 4716 4717 4718 4719 4720 4721 4722 4723 4724 \
		4725 4726 4727 4728 4729 4731 4732 4733 4734 4739 4740 4741 4742 4743 4744 4745 4746 4747 4749 4750 \
		4751 4752 4753 4754 4755 4756 4757 4761 4764 4771 4772 4775 4776 4777 4778 4786 4788 4789 4792 4793 \
		4794 4798 4799 4800 4801 4803 4804 4805 4806 4807 4808 4809 4810 4811 4812 4813 4815 4816 4817 4821 \
		4822 4823 4827 4829 4834 4835 4839 4840 4841 4842 4843 4844 4845 4846 4847 4848 4849 4867 4869 4872 \
		4880 4881 4882 4883 4900 4905 4906 4910 4912 4913 4916 4917 4918 4920 4921 4925 4926 4927 4928 4929 \
		4930 4931 4932 4934 4935 4936 4937 4938 4939 4944 4945 4946 4947 4948 4949 4950 4951 4952 4953 4954 \
		4955 4956 4957 4958 4959 4960 4961 4963 4964 4965 4966 4970 4971 4972 4973 4974 4981 4985 4989 4990 \
		4991 4992 4995 4997 4998 4999 5038
	#pragma warning (error: __WARNINGS_AS_ERRORS)


	// Disable trivial warnings

	#pragma warning(disable: 4068)  // 'unknown pragma' :
	#pragma warning(disable: 4091)  // 'keyword' : ignored on left of 'type' when no variable is declared
	#pragma warning(disable: 4100)  // 'identifier' : unreferenced formal parameter
	#pragma warning(disable: 4121)  // 'symbol' : alignment of a member was sensitive to packing
	#pragma warning(disable: 4127)  // Conditional expression is constant
	#pragma warning(disable: 4180)  // qualifier applied to function type has no meaning; ignored
	#pragma warning(disable: 4189)  // 'identifier': local variable is initialized but not referenced
	#pragma warning(disable: 4200)  // Zero-length array item at end of structure, a VC-specific extension
	#pragma warning(disable: 4201)  // nonstandard extension used : nameless struct/union
	#pragma warning(disable: 4217)  // 'operator' : member template functions cannot be used for copy-assignment or copy-construction
	#pragma warning(disable: 4245)  // 'initializing': conversion from 'type' to 'type', signed/unsigned mismatch
	#pragma warning(disable: 4251)  // 'type' needs to have dll-interface to be used by clients of 'type'
	#pragma warning(disable: 4267)  // 'var' : conversion from 'size_t' to 'type', possible loss of data
	#pragma warning(disable: 4275)  // non - DLL-interface classkey 'identifier' used as base for DLL-interface classkey 'identifier'
	#pragma warning(disable: 4291)  // typedef-name '' used as synonym for class-name ''
	#pragma warning(disable: 4307)  // '': integral constant overflow
	#pragma warning(disable: 4315)  // 'classname': 'this' pointer for member 'member' may not be aligned 'alignment' as expected by the constructor
	#pragma warning(disable: 4316)  // 'identifier': object allocated on the heap may not be aligned 'alignment'
	#pragma warning(disable: 4324)  // structure was padded due to __declspec(align())
	#pragma warning(disable: 4347)  // behavior change: 'function template' is called instead of 'function
	#pragma warning(disable: 4351)  // new behavior: elements of array 'array' will be default initialized
	#pragma warning(disable: 4355)  // this used in base initializer list
	#pragma warning(disable: 4366)  // The result of the unary 'operator' operator may be unaligned
	#pragma warning(disable: 4373)  // '%$S': virtual function overrides '%$pS', previous versions of the compiler did not override when parameters only differed by const/volatile qualifiers
	#pragma warning(disable: 4389)  // signed/unsigned mismatch
	#pragma warning(disable: 4464)  // relative include path contains '..'
	#pragma warning(disable: 4482)  // nonstandard extension used: enum 'enumeration' used in qualified name
	#pragma warning(disable: 4505)  // 'function' : unreferenced local function has been removed
	#pragma warning(disable: 4511)  // 'class' : copy constructor could not be generated
	#pragma warning(disable: 4512)  // 'class' : assignment operator could not be generated
	#pragma warning(disable: 4514)  // 'function' : unreferenced inline function has been removed
	#pragma warning(disable: 4599)  // 'flag path': command line argument number number does not match precompiled header
	#pragma warning(disable: 4605)  // '/Dmacro' specified on current command line, but was not specified when precompiled header was built
	#pragma warning(disable: 4623)  // 'derived class' : default constructor was implicitly defined as deleted because a base class default constructor is inaccessible or deleted
	#pragma warning(disable: 4625)  // 'derived class' : copy constructor was implicitly defined as deleted because a base class copy constructor is inaccessible or deleted
	#pragma warning(disable: 4626)  // 'derived class' : assignment operator was implicitly defined as deleted because a base class assignment operator is inaccessible or deleted
	#pragma warning(disable: 4640)  // 'instance' : construction of local static object is not thread-safe
	#pragma warning(disable: 4699)  // creating precompiled header
	#pragma warning(disable: 4702)  // unreachable code
	#pragma warning(disable: 4710)  // 'function' : function not inlined
	#pragma warning(disable: 4711)  // function selected for automatic inlining
	#pragma warning(disable: 4714)  // function 'function' marked as __forceinline not inlined
	#pragma warning(disable: 4748)  // /GS can not protect parameters and local variables from local buffer overrun because optimizations are disabled in function
	#pragma warning(disable: 4768)  // __declspec attributes before linkage specification are ignored
	#pragma warning(disable: 4799)  // Warning: function 'ident' has no EMMS instruction
	#pragma warning(disable: 4828)  // The file contains a character starting at offset ... that is illegal in the current source character set(codepage ...).
	#pragma warning(disable: 4868)  // 'file(line_number)' compiler may not enforce left-to-right evaluation order in braced initializer list
	#pragma warning(disable: 4917)  // 'declarator' : a GUID can only be associated with a class, interface or namespace


	// Override previous error setting for specific warnings.
	// We can be a little lazy, but they really should get fixed

	#pragma warning(default: 4191)  // 'operator/operation': unsafe conversion from 'type_of_expression' to 'type_required'
	#pragma warning(default: 4255)  // 'function' : no function prototype given: converting '()' to '(void)'
	#pragma warning(default: 4263)  // 'function' : member function does not override any base class virtual member function
	#pragma warning(default: 4264)  // 'virtual_function' : no override available for virtual member function from base 'class'; function is hidden
	#pragma warning(default: 4265)  // 'class' : class has virtual functions, but destructor is not virtual
	#pragma warning(default: 4287)  // 'operator' : unsigned/negative constant mismatch
	#pragma warning(default: 4289)  // nonstandard extension used : 'var' : loop control variable declared in the for-loop is used outside the for-loop scope
	#pragma warning(disable: 4345)  // behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
	#pragma warning(disable: 4514)  // unreferenced inline/local function has been removed
	#pragma warning(default: 4529)  // 'member_name' : forming a pointer-to-member requires explicit use of the address-of operator ('&') and a qualified name
	#pragma warning(default: 4536)  // 'type name' : type-name exceeds meta-data limit of 'limit' characters
	#pragma warning(default: 4545)  // expression before comma evaluates to a function which is missing an argument list
	#pragma warning(default: 4546)  // function call before comma missing argument list
	#pragma warning(default: 4557)  // '__assume' contains effect 'effect'
	#pragma warning(disable: 4577)  // 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed. Specify /EHsc
	#pragma warning(default: 4592)  // 'function': 'constexpr' call evaluation failed; function will be called at run-time
	#pragma warning(default: 4628)  // digraphs not supported with -Ze. Character sequence 'digraph' not interpreted as alternate token for 'char'
	#pragma warning(default: 4682)  // 'parameter' : no directional parameter attribute specified, defaulting to [in]
	#pragma warning(default: 4686)  // 'user-defined type' : possible change in behavior, change in UDT return calling convention
	#pragma warning(disable: 4710)  // 'function' : function not inlined / The given function was selected for inline expansion, but the compiler did not perform the inlining.
	#pragma warning(default: 4786)  // 'identifier' : identifier was truncated to 'number' characters in the debug information
	#pragma warning(default: 4793)  // 'function' : function is compiled as native code: 'reason'
	#pragma warning(default: 4905)  // wide string literal cast to 'LPSTR'
	#pragma warning(default: 4906)  // string literal cast to 'LPWSTR'
	#pragma warning(default: 4928)  // illegal copy-initialization; more than one user-defined conversion has been implicitly applied
	#pragma warning(default: 4931)  // we are assuming the type library was built for number-bit pointers
	#pragma warning(default: 4946)  // reinterpret_cast used between related classes: 'class1' and 'class2'
	#pragma warning(disable: 4984)  // 'if constexpr' is a C++17 language extension
	#pragma warning(default: 4996)  // code uses a function, class member, variable, or typedef that's marked deprecated
	#pragma warning(default: 5038)  // data member 'A::y' will be initialized after data member 'A::x'


	//---------------------------------------------------------
	// Preprocessor helper for including third-party libraries
	// that doesn't play nice with current compiler warnings

	#define BEGIN_INCLUDE_DEPENDENCIES                                                                                                                                                                              \
		__pragma(pack(push))                                                                                                                                                                                        \
		__pragma(warning(push))                                                                                                                                                                                     \
		__pragma(warning(disable: 4191))  /* 'operator/operation': unsafe conversion from 'type_of_expression' to 'type_required' */                                                                                \
		__pragma(warning(disable: 4244))  /* 'conversion' conversion from 'type1' to 'type2', possible loss of data */                                                                                              \
		__pragma(warning(disable: 4310))  /* cast truncates constant value */                                                                                                                                       \
		__pragma(warning(disable: 4456))  /* declaration of 'LocalVariable' hides previous local declaration */                                                                                                     \
		__pragma(warning(disable: 4457))  /* declaration of 'LocalVariable' hides function parameter */                                                                                                             \
		__pragma(warning(disable: 4458))  /* declaration of 'LocalVariable' hides class member */                                                                                                                   \
		__pragma(warning(disable: 4459))  /* declaration of 'LocalVariable' hides global declaration */                                                                                                             \
		__pragma(warning(disable: 4510))  /* '<class>': default constructor could not be generated. */                                                                                                              \
		__pragma(warning(disable: 4610))  /* object '<class>' can never be instantiated - user-defined constructor required. */                                                                                     \
		__pragma(warning(disable: 4668))  /* 'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives' */                                                                                \
		__pragma(warning(disable: 4800))  /* Implicit conversion from '<type>' to bool. Possible information loss. */                                                                                               \
		__pragma(warning(disable: 4946))  /* reinterpret_cast used between related classes: '<class1>' and '<class1>' */                                                                                            \
		__pragma(warning(disable: 4995))  /* 'function': name was marked as #pragma deprecated */                                                                                                                   \
		__pragma(warning(disable: 4996))  /* The compiler encountered a deprecated declaration. */                                                                                                                  \
		__pragma(warning(disable: 5038))  /* data member 'member1' will be initialized after data member 'member2' data member 'member' will be initialized after base class 'base_class' */                        \
		__pragma(warning(disable: 6011))  /* Dereferencing NULL pointer '<ptr>'. */                                                                                                                                 \
		__pragma(warning(disable: 6101))  /* Returning uninitialized memory '<expr>'.  A successful path through the function does not set the named _Out_ parameter. */                                            \
		__pragma(warning(disable: 6244))  /* local declaration of <variable> hides previous declaration at <line> of <file> */                                                                                      \
		__pragma(warning(disable: 6287))  /* Redundant code:  the left and right sub-expressions are identical. */                                                                                                  \
		__pragma(warning(disable: 6308))  /* 'realloc' might return null pointer: assigning null pointer to 'X', which is passed as an argument to 'realloc', will cause the original memory block to be leaked. */ \
		__pragma(warning(disable: 6326))  /* Potential comparison of a constant with another constant. */                                                                                                           \
		__pragma(warning(disable: 6340))  /* Mismatch on sign: Incorrect type passed as parameter in call to function. */                                                                                           \
		__pragma(warning(disable: 6385))  /* Reading invalid data from '<ptr>':  the readable size is '<num1>' bytes, but '<num2>' bytes may be read. */                                                            \
		__pragma(warning(disable: 6386))  /* Buffer overrun while writing to '<ptr>':  the writable size is '<num1>' bytes, but '<num2>' bytes might be written. */                                                 \
		__pragma(warning(disable: 28182)) /* Dereferencing NULL pointer. '<ptr1>' contains the same NULL value as '<ptr2>' did. */                                                                                  \
		__pragma(warning(disable: 28251)) /* Inconsistent annotation for '<func>': this instance has no annotations. */                                                                                             \
		__pragma(warning(disable: 28252)) /* Inconsistent annotation for '<func>': return/function has '<annotation>' on the prior instance. */                                                                     \
		__pragma(warning(disable: 28253)) /* Inconsistent annotation for '<func>': _Param_(<num>) has '<annotation>' on the prior instance. */                                                                      \
		__pragma(warning(disable: 28301)) /* No annotations for first declaration of '<func>'. */

	#define END_INCLUDE_DEPENDENCIES                                                                                                 \
		__pragma(warning(pop))                                                                                                       \
		__pragma(pack(pop))

#endif // _MSC_VER
// clang-format on

//---------------------------------------------------------
// Windows specific types

namespace iplug::Types
{
	using Platform = struct Windows : public Generic
	{
		using uint8  = std::uint8_t;
		using uint16 = std::uint16_t;
		using uint32 = std::uint32_t;
		using uint64 = std::uint64_t;
		using int8   = std::int8_t;
		using int16  = std::int16_t;
		using int32  = std::int32_t;
		using int64  = std::int64_t;
	};
}  // namespace iplug::Types


//---------------------------------------------------------
// Configure and include windows.h
// TODO: Remove this include file when isolation is complete

#include "WindowsSDK.h"
