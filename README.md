# Script_G3Fixes
This script fixes few things in game and also adds new features


# Compiled binaries  
[<img src="https://img.shields.io/badge/here-grey?logo=GoogleDrive&logoColor=4285F4&style=for-the-badge">](https://drive.google.com/drive/folders/1vQGQMim-WsOKXqo1wfm7i2uZS3OMC_MU)

# Details  
#### New, more random percentage chance for applying status effects like Freeze or Burn on hit
- Spells:  
  Area spells or charged spells have 100% to apply effect  
  Quick casted spells have 10% chance to apply effect (optional, QuickCastChance in config)  
- Melee:  
  Normal attacks have 30% chance to apply  
  Power attacks have 35% chance to apply  
- Ranged:  
  Based on the bow's tension force  
#### Companions that are following Hero will now attack enemies immediately when Hero or other companion is targeted and attacked (optional, CompanionAutoDefend in config)  
#### Companions will now use sprint when following Hero  
#### Companions that are too far away from Hero will be teleported near Hero, distance for this check is Entity.ROI - 250.0 (optional, TeleportCompanionTooFarAway in config)  
#### If Player cancels routine of an NPC (by walking into the room or drawing a weapon) that is using interactive objects like beds, bookstands, NPC will try to take a few steps back and free the object, so Hero can use it now  
#### Added console command for reviving NPCs  
  `revive NPC_INSTANCE` or simply `revive` when dead NPC is focused (it can only be focused if corpse was not looted yet)  
#### Added console command for resetting NPCs position  
  `resetpos NPC_INSTANCE` or simply `resetpos` when NPC is focused and not dead
  NPC should be teleported to the destination point of their current routine
#### Fully integrated Script_ItemUseFuncEnabler  
(this feature is for mod developers, example modified working templates for Cure Disease and Antidote are included in optional zip file)  
# Installation  
Extract Script_G3Fixes.zip inside main directory of Gothic 3 (where Gothic3.exe is placed).  

If you want Cure Disease potion and Antidote to work without that stupid spell animation, extract also Script_G3Fixes_Example_Templates.zip inside main directory.
