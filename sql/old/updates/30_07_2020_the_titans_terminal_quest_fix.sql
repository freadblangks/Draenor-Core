DELETE FROM `spell_script_names` WHERE `spell_id` = '65300';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(65300, 'spell_ping_for_artifacts');

DELETE FROM `gameobject_loot_template` WHERE `item` = '46702';
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountorref`, `maxcount`, `itembonuses`) VALUES
(27251, 46702, -44, 1, 0, 1, 1, '');

-- Buried Treasure Bunny
SET @ENTRY := 34356;
SET @SOURCETYPE := 0;

DELETE FROM `smart_scripts` WHERE `entryorguid`=@ENTRY AND `source_type`=@SOURCETYPE;
UPDATE creature_template SET AIName="SmartAI" WHERE entry=@ENTRY LIMIT 1;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
(@ENTRY,@SOURCETYPE,0,0,8,0,100,0,65300,1,0,0,11,65306,2,0,0,0,0,1,0,0,0,0.0,0.0,0.0,0.0,"On Spellhit - Summon Buried Treasure");