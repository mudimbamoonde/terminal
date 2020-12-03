﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "Profiles.h"
#include "Profiles.g.cpp"
#include "EnumEntry.h"

#include <LibraryResources.h>

using namespace winrt::Windows::UI::Text;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Navigation;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::AccessCache;
using namespace winrt::Windows::Storage::Pickers;
using namespace winrt::Microsoft::Terminal::Settings::Model;

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    Profiles::Profiles() :
        _ColorSchemeList{ single_threaded_observable_vector<ColorScheme>() }
    {
        InitializeComponent();

        INITIALIZE_BINDABLE_ENUM_SETTING(CursorShape, CursorStyle, winrt::Microsoft::Terminal::TerminalControl::CursorStyle, L"Profile_CursorShape", L"Content");
        INITIALIZE_BINDABLE_ENUM_SETTING(BackgroundImageStretchMode, BackgroundImageStretchMode, winrt::Windows::UI::Xaml::Media::Stretch, L"Profile_BackgroundImageStretchMode", L"Content");
        INITIALIZE_BINDABLE_ENUM_SETTING(AntiAliasingMode, TextAntialiasingMode, winrt::Microsoft::Terminal::TerminalControl::TextAntialiasingMode, L"Profile_AntialiasingMode", L"Content");
        INITIALIZE_BINDABLE_ENUM_SETTING(CloseOnExitMode, CloseOnExitMode, winrt::Microsoft::Terminal::Settings::Model::CloseOnExitMode, L"Profile_CloseOnExit", L"Content");
        INITIALIZE_BINDABLE_ENUM_SETTING(BellStyle, BellStyle, winrt::Microsoft::Terminal::Settings::Model::BellStyle, L"Profile_BellStyle", L"Content");
        INITIALIZE_BINDABLE_ENUM_SETTING(ScrollState, ScrollbarState, winrt::Microsoft::Terminal::TerminalControl::ScrollbarState, L"Profile_ScrollbarVisibility", L"Content");

        // manually add Custom FontWeight option. Don't add it to the Map
        INITIALIZE_BINDABLE_ENUM_SETTING(FontWeight, FontWeight, uint16_t, L"Profile_FontWeight", L"Content");
        _CustomFontWeight = winrt::make<EnumEntry>(RS_(L"Profile_FontWeightCustom/Content"), winrt::box_value<uint16_t>(0u));
        _FontWeightList.Append(_CustomFontWeight);
    }

    void Profiles::OnNavigatedTo(const NavigationEventArgs& e)
    {
        _State = e.Parameter().as<Editor::ProfilePageNavigationState>();

        const auto& colorSchemeMap{ _State.Schemes() };
        for (const auto& pair : colorSchemeMap)
        {
            _ColorSchemeList.Append(pair.Value());
        }
    }

    ColorScheme Profiles::CurrentColorScheme()
    {
        const auto schemeName{ _State.Profile().ColorSchemeName() };
        if (const auto scheme{ _State.Schemes().TryLookup(schemeName) })
        {
            return scheme;
        }
        else
        {
            // This Profile points to a color scheme that was renamed or deleted.
            // Fallback to Campbell.
            return _State.Schemes().TryLookup(L"Campbell");
        }
    }

    void Profiles::CurrentColorScheme(const ColorScheme& val)
    {
        _State.Profile().ColorSchemeName(val.Name());
    }

    fire_and_forget Profiles::BackgroundImage_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto lifetime = get_strong();

        FileOpenPicker picker;

        _State.WindowRoot().TryPropagateHostingWindow(picker); // if we don't do this, there's no HWND for it to attach to
        picker.ViewMode(PickerViewMode::Thumbnail);
        picker.SuggestedStartLocation(PickerLocationId::PicturesLibrary);
        picker.FileTypeFilter().ReplaceAll({ L".jpg", L".jpeg", L".png", L".gif" });

        StorageFile file = co_await picker.PickSingleFileAsync();
        if (file != nullptr)
        {
            BackgroundImage().Text(file.Path());
        }
    }

    fire_and_forget Profiles::Commandline_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto lifetime = get_strong();

        FileOpenPicker picker;

        //TODO: SETTINGS UI Commandline handling should be robust and intelligent
        _State.WindowRoot().TryPropagateHostingWindow(picker); // if we don't do this, there's no HWND for it to attach to
        picker.ViewMode(PickerViewMode::Thumbnail);
        picker.SuggestedStartLocation(PickerLocationId::ComputerFolder);
        picker.FileTypeFilter().ReplaceAll({ L".bat", L".exe" });

        StorageFile file = co_await picker.PickSingleFileAsync();
        if (file != nullptr)
        {
            Commandline().Text(file.Path());
        }
    }

    fire_and_forget Profiles::StartingDirectory_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto lifetime = get_strong();
        FolderPicker picker;
        _State.WindowRoot().TryPropagateHostingWindow(picker); // if we don't do this, there's no HWND for it to attach to
        picker.SuggestedStartLocation(PickerLocationId::DocumentsLibrary);
        picker.FileTypeFilter().ReplaceAll({ L"*" });
        StorageFolder folder = co_await picker.PickSingleFolderAsync();
        if (folder != nullptr)
        {
            StorageApplicationPermissions::FutureAccessList().AddOrReplace(L"PickedFolderToken", folder);
            StartingDirectory().Text(folder.Path());
        }
    }

    IInspectable Profiles::CurrentFontWeight() const
    {
        // if no value was found, we have a custom value
        const auto maybeEnumEntry{ _FontWeightMap.TryLookup(_State.Profile().FontWeight().Weight) };
        return maybeEnumEntry ? maybeEnumEntry : _CustomFontWeight;
    }

    void Profiles::CurrentFontWeight(const IInspectable& enumEntry)
    {
        if (auto ee = enumEntry.try_as<Editor::EnumEntry>())
        {
            if (ee != _CustomFontWeight)
            {
                const auto weight{ winrt::unbox_value<uint16_t>(ee.EnumValue()) };
                const Windows::UI::Text::FontWeight setting{ weight };
                _State.Profile().FontWeight(setting);

                // Profile does not have observable properties
                // So the TwoWay binding doesn't update on the State --> Slider direction
                FontWeightSlider().Value(weight);
            }
            _PropertyChangedHandlers(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{ L"IsCustomFontWeight" });
        }
    }

    bool Profiles::IsCustomFontWeight()
    {
        // Use SelectedItem instead of CurrentFontWeight.
        // CurrentFontWeight converts the Profile's value to the appropriate enum entry,
        // whereas SelectedItem identifies which one was selected by the user.
        return FontWeightComboBox().SelectedItem() == _CustomFontWeight;
    }
}