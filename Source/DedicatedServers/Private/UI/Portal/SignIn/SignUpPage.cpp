// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Portal/SignIn/SignUpPage.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"

void USignUpPage::UpdateStatusMessage(const FString& Message, bool bShouldResetWidgets)
{
	TextBlock_StatusMessage->SetText(FText::FromString(Message));
	if (bShouldResetWidgets)
	{
		Button_SignUp->SetIsEnabled(true);
	}
}

void USignUpPage::NativeConstruct()
{
	Super::NativeConstruct();
	
	TextBox_UserName->OnTextChanged.AddDynamic(this, &USignUpPage::UpdateSignUpButtonState);
	TextBox_Password->OnTextChanged.AddDynamic(this, &USignUpPage::UpdateSignUpButtonState);
	TextBox_ConfirmPassword->OnTextChanged.AddDynamic(this, &USignUpPage::UpdateSignUpButtonState);
	TextBox_Email->OnTextChanged.AddDynamic(this, &USignUpPage::UpdateSignUpButtonState);
	Button_SignUp->SetIsEnabled(false);
}

void USignUpPage::UpdateSignUpButtonState(const FText& Text)
{
	const bool bIsUsernameValid = !TextBox_UserName->GetText().IsEmpty();
	const bool bArePasswordsEqual = TextBox_Password->GetText().ToString() == TextBox_ConfirmPassword->GetText().ToString();
	const bool bIsValidEmail = IsValidEmail(TextBox_Email->GetText().ToString());
	const bool bIsPasswordLongEnough = TextBox_Password->GetText().ToString().Len() >= 8;
	
	FString StatusMessage;
	const bool bIsStrongPassword = IsStrongPassword(TextBox_Password->GetText().ToString(), StatusMessage);
	if (!bIsStrongPassword)
	{
		TextBlock_StatusMessage->SetText(FText::FromString(StatusMessage));
	}
	else if (!bIsUsernameValid)
	{
		TextBlock_StatusMessage->SetText(FText::FromString(TEXT("Please enter a valid Username")));
	}
	else if (!bArePasswordsEqual)
	{
		TextBlock_StatusMessage->SetText(FText::FromString(TEXT("Passwords do not match.")));
	}
	else if (!bIsValidEmail)
	{
		TextBlock_StatusMessage->SetText(FText::FromString(TEXT("Please enter a valid Email address.")));
	}
	else if (!bIsPasswordLongEnough)
	{
		TextBlock_StatusMessage->SetText(FText::FromString(TEXT("Password must be at least 8 characters long.")));
	}
	else
	{
		TextBlock_StatusMessage->SetText(FText::GetEmpty());
	}
	Button_SignUp->SetIsEnabled(bIsUsernameValid && bArePasswordsEqual && bIsValidEmail && bIsPasswordLongEnough && bIsStrongPassword);
}

bool USignUpPage::IsValidEmail(const FString& Email)
{
	const FRegexPattern EmailPattern(TEXT(R"((^[^\s@]+@[^\s@]+\.[^\s@]{2,}$))"));
	
	FRegexMatcher Matcher(EmailPattern, Email);
	return Matcher.FindNext();
}

bool USignUpPage::IsStrongPassword(const FString& Password, FString& StatusMessage)
{
	const FRegexPattern NumberPatern(TEXT(R"(\d)")); // contains at least one number
	const FRegexPattern SpecialCharPatern(TEXT(R"([^\w\s])")); // contains at least one special character
	const FRegexPattern UppercasePatern(TEXT(R"([A-Z])")); // contains at least one uppercase letter
	const FRegexPattern LowercasePatern(TEXT(R"([a-z])")); // contains at least one lowercase letter
	
	FRegexMatcher NumberMatcher(NumberPatern, Password);
	FRegexMatcher SpecialCharMatcher(SpecialCharPatern, Password);
	FRegexMatcher UppercaseMatcher(UppercasePatern, Password);
	FRegexMatcher LowercaseMatcher(LowercasePatern, Password);
	
	if (!NumberMatcher.FindNext())
	{
		StatusMessage = TEXT("Password must contain at least one number.");
		return false;
	}
	if (!SpecialCharMatcher.FindNext())
	{
		StatusMessage = TEXT("Password must contain at least one special character.");
		return false;
	}
	if (!UppercaseMatcher.FindNext())
	{
		StatusMessage = TEXT("Password must contain at least one uppercase letter.");
		return false;
	}
	if (!LowercaseMatcher.FindNext())
	{
		StatusMessage = TEXT("Password must contain at least one lowercase letter.");
		return false;
	}
	return true;
}
