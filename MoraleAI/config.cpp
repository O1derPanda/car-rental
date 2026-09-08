class CfgPatches
{
	class MoraleAI
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Data",
			"DZ_Scripts"
		};
	};
};

class CfgMods
{
	class MoraleAI
	{
		dir="MoraleAI";
		picture="";
		action="";
		hideName=1;
		hidePicture=1;
		name="MoraleAI";
		credits="";
		author="Jules";
		authorID="0";
		version="1.0";
		extra=0;
		type="mod";
		dependencies[]=
		{
			"Game",
			"World",
			"Mission"
		};
		class defs
		{
			class gameScriptModule
			{
				value="";
				files[]=
				{
					"MoraleAI/Scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"MoraleAI/Scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"MoraleAI/Scripts/5_Mission"
				};
			};
		};
	};
};
class CfgVehicles
{
	class SurvivorM_Mirek;
	class MoraleAIBotBase: SurvivorM_Mirek
	{
		scope=2;
		displayName="Morale AI Bot";
		descriptionShort="An AI bot with a morale system.";
	};
};
