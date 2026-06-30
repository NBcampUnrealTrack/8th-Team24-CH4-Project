// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CrazyRoo : ModuleRules
{
	public CrazyRoo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			//"OnlineSubsystem", // 주요 기능: 방 만들기(Create Session), 방 찾기(Find Session), 방 참가하기(Join Session), 친구 목록 불러오기, 업적 달성, 리더보드(랭킹) 등의 기능
			//"OnlineSubsystemUtils" //OnlineSubsystem 편하게 쓰기 위한 기능
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"CrazyRoo",
			"CrazyRoo/Public/System",//
			"CrazyRoo/Public/Player",//
			"CrazyRoo/Private/System",//
			"CrazyRoo/Private/Player",//
			"CrazyRoo/Variant_Platforming",
			"CrazyRoo/Variant_Platforming/Animation",
			"CrazyRoo/Variant_Combat",
			"CrazyRoo/Variant_Combat/AI",
			"CrazyRoo/Variant_Combat/Animation",
			"CrazyRoo/Variant_Combat/Gameplay",
			"CrazyRoo/Variant_Combat/Interfaces",
			"CrazyRoo/Variant_Combat/UI",
			"CrazyRoo/Variant_SideScrolling",
			"CrazyRoo/Variant_SideScrolling/AI",
			"CrazyRoo/Variant_SideScrolling/Gameplay",
			"CrazyRoo/Variant_SideScrolling/Interfaces",
			"CrazyRoo/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
