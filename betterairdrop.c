/*
 * DayZ 0.63 BetterAirdrop by philippj
 * inspired by AirDrop from mov3ax

	Installation:
	- Create "plugins" directory in your missions folder & place this script there
	- Edit "init.c" add at the top
	#include "$CurrentDir:\\mpmissions\\dayzOffline.chernarusplus\\plugins\\betterairdrop.c"
	
	IF YOU DONT HAVE A CUSTOM MISSION CONSTRUCTOR
	- Add following at the top of class CustomMission: MissionServer
	ref BetterAirdrop Airdrop;
    void CustomMission() {
        Airdrop = new BetterAirdrop();
    }

	IF YOU HAVE A CUSTOM MISSION CONSTRUCTOR
	- Add following at the top of class CustomMission: MissionServer
	ref BetterAirdrop Airdrop;
	- Add following to your constructor
	Airdrop = new BetterAirdrop();
 */

class BetterAirdropLocation {
    float x, y;
    string name;
    bool disclose, send_proximity;

    void BetterAirdropLocation(float x, float y, string name, bool disclose = false, bool send_proximity = false) {
        this.x = x;
        this.y = y;
        this.name = name;
        this.disclose = disclose;
        this.send_proximity = send_proximity;
    }
};

class BetterAirdrop {
	

	/*
	 * 		Config
	 */

    bool debug_mode = false; // display d2t & port to target

	float interval = 60.0; // in minutes
    float initial = 60.0; // in minutes
	float plane_height = 300; // relative to ground
    float proximity_warning_distance = 1500.0;

    float speed = 1.0; // Higher then 2 looks odd

	bool smoke_signal = true; // Not working

    int infected_count = 15;
    int items_to_be_spawned = 7;

    int number_of_drops = 1; // number of drops must match the number amount of airdrop locations
    ref BetterAirdropLocation airdrop_locations[] = {
        new BetterAirdropLocation(4807.88, 9812.09, "Northwest  Airfield", true, true),
    };

	/*
	 * internals
	 */

    int motionless_ticks = 0;

	Object plane;
	vector plane_pos;
    vector spawn_pos;
    vector drop_pos;
    vector orientation;

    House airdrop_obj;
    Object tmp_airdrop_obj;
    EntityAI airdrop_physics;

    Particle signal;
    Particle explosion;
    Particle smoke;

    bool active = false;
    bool dropped = false;
    bool sent_proximity_warning = false;

    ref BetterAirdropLocation active_drop;

	void BetterAirdrop() {
		GetGame().AdminLog("<BetterAirdrop> plugin init");
        ResetPlane();
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(InitiateAirdrop, initial*60*1000, false);
	}

    void InitiateAirdrop() {
        ResetPlane();
        Cleanup();
        Spawn();
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(InitiateAirdrop, interval*60*1000, false);
    }

    void Cleanup() {
        GetGame().ObjectDelete(airdrop_obj);
    }

    void ResetPlane() {
        GetGame().ObjectDelete(plane);
        plane = GetGame().CreateObject( "Land_Wreck_C130J", "0 0 0", false, true, false );
        plane_pos[0] = 0;
        plane_pos[1] = 0;
        plane_pos[2] = 0;
        plane.SetPosition(plane_pos);
        dropped = false;
        active = false;
        sent_proximity_warning = false;
        if(active_drop != NULL && active_drop.send_proximity) SendMessageToPlayers("The plane has vanished");
    }

    void MovePlane() {
        float angle = Math.Atan2(drop_pos[1]-spawn_pos[1], drop_pos[0]-spawn_pos[0]);
        float fixed_angle = angle*Math.RAD2DEG;

        plane_pos[0] = Math.Cos(angle);
        plane_pos[2] = Math.Sin(angle);
        vector advanced_plane_pos = plane.GetPosition() + (plane_pos*speed);

        advanced_plane_pos[1] = GetGame().SurfaceY(advanced_plane_pos[0], advanced_plane_pos[2]) + plane_height;

        plane.SetPosition(advanced_plane_pos);
        //TeleportPlayers(advanced_plane_pos + "10 10 10");
        if(!dropped) {
            float distance = Math.Sqrt(Math.Pow(drop_pos[0] - advanced_plane_pos[0], 2) + Math.Pow(drop_pos[1] - advanced_plane_pos[2], 2));
            if(debug_mode) SendMessageToPlayers("DISTANCE2TARGET: "+distance);
            if(distance <= 10.0) {
                dropped = true;
                Drop();
                GetGame().AdminLog("<BetterAirdrop> drop ("+active_drop.name+")"); // Used for CFTools - OmegaSC
                if(active_drop.send_proximity) SendMessageToPlayers("<BetterAirdrop> The supplies have been dropped around " + active_drop.name);
            } else if(distance <= proximity_warning_distance && !sent_proximity_warning) {
                sent_proximity_warning = true;
                GetGame().AdminLog("<BetterAirdrop> proximity ("+active_drop.name+")"); // Used for CFTools - OmegaSC
                if(active_drop.send_proximity) SendMessageToPlayers("<BetterAirdrop> The plane is closing in on " + active_drop.name);
            }
        } else {
            if(!IsInRect(advanced_plane_pos[0], advanced_plane_pos[1], -10.0, 15470.0, -10.0, 1570.0)) {
                GetGame().AdminLog("<BetterAirdrop> cleaning up");
                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(MovePlane);
                GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ResetPlane, 1000, false);
            }
        }
    }
    
    void DropSimulation() {
        float ground = GetGame().SurfaceY(airdrop_obj.GetPosition()[0], airdrop_obj.GetPosition()[2]);
        float diff = airdrop_obj.GetPosition()[1]-airdrop_physics.GetPosition()[1];
        if(diff <= 0.0001) motionless_ticks += 1;
        else motionless_ticks = 0;
        if (airdrop_physics.GetPosition()[1] <= (ground + 3.0) || motionless_ticks >= 25) { // force drop if stuck for more then 500ms
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(DropSimulation);
            vector tmp_pos = airdrop_obj.GetPosition();
            tmp_pos[1] = ground;
            GetGame().ObjectDelete(airdrop_physics);
            airdrop_obj.SetPosition(tmp_pos);
            airdrop_obj.PlaceOnSurface();
            GetGame().AdminLog("<BetterAirdrop> hit surface");
            AfterDrop();
        } else {
            vector airdrop_obj_pos;
            airdrop_obj_pos[0] = drop_pos[0];
            airdrop_obj_pos[1] = airdrop_physics.GetPosition()[1];
            airdrop_obj_pos[2] = drop_pos[1];
            airdrop_obj.SetPosition(airdrop_obj_pos);
            airdrop_obj.SetOrientation(airdrop_physics.GetOrientation());
        }
    }
    
    
    void AfterDrop() {
        vector base_pos = airdrop_obj.GetPosition(), dynamic_pos;
        for(int i = 0; i < items_to_be_spawned; i++) {
            float a = Math.RandomFloat(0.4, 1.0) * 2 * Math.PI;
            float r = 5.0 * Math.Sqrt(Math.RandomFloat(0.4, 1.0));
            dynamic_pos = base_pos;
            dynamic_pos[0] = dynamic_pos[0]+(r * Math.Cos(a));
            dynamic_pos[2] = dynamic_pos[2]+(r * Math.Sin(a));
            dynamic_pos[1] = GetGame().SurfaceY(dynamic_pos[0], dynamic_pos[2]) + 0.3;
            string item = GetRandomItem();
            GetGame().CreateObject(item, dynamic_pos, false, true);
        }
        for ( int inf = 0; inf < infected_count; inf++ ) {
            dynamic_pos = base_pos;
            dynamic_pos[0] = dynamic_pos[0]+Math.RandomFloat(-20.0, 20.0);
            dynamic_pos[2] = dynamic_pos[2]+Math.RandomFloat(-20.0, 20.0);
            GetGame().CreateObject( WorkingZombieClasses().GetRandomElement(), dynamic_pos, false, true );
        }
    }
    

    void Drop() {
        motionless_ticks = 0;
        GetGame().AdminLog("<BetterAirdrop> initiated drop physics");
        airdrop_physics = EntityAI.Cast(GetGame().CreateObject( "CivilianSedan", plane.GetPosition(), true, true ));
        airdrop_physics.SetAllowDamage(false);
        airdrop_obj = GetGame().CreateObject( "Land_Container_1Bo", plane.GetPosition(), false, true );
        SetVelocity(airdrop_physics, "10 0 0");
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(DropSimulation, 10, true);
    }

	void Spawn() {
        GetGame().AdminLog("<BetterAirdrop> airdrop init");

        int side = Math.RandomInt(0,4);
        switch(side) {
            case 0: {
                spawn_pos[0] = 0.0;
                spawn_pos[1] = Math.RandomFloat(0.0, 15360.0);
                break;
            }
            case 1: {
                spawn_pos[0] = 15360.0;
                spawn_pos[1] = Math.RandomFloat(0.0, 15360.0);
                break;
            }
            case 2: {
                spawn_pos[0] = Math.RandomFloat(0.0, 15360.0);
                spawn_pos[1] = 0.0;
                break;
            }
            case 3: {
                spawn_pos[0] = Math.RandomFloat(0.0, 15360.0);
                spawn_pos[1] = 15360.0;
                break;
            }
        }

        vector plane_start_pos;
        plane_start_pos[0] = spawn_pos[0];
        plane_start_pos[2] = spawn_pos[1];
        plane.SetPosition(plane_start_pos);

        BetterAirdropLocation default_drop = new BetterAirdropLocation(2760.0, 5527.0, "ravioli ravioli gib mir die formeloli", true, true);
        if(number_of_drops < 1) {
            GetGame().AdminLog("<BetterAirdrop> No random locations defined, using fallback");
            active_drop = default_drop;
        } else {
            active_drop = airdrop_locations[Math.RandomInt(0, number_of_drops - 1)];
        }
        vector tmp_pos;
        tmp_pos[0] = active_drop.x;
        tmp_pos[1] = GetGame().SurfaceY(active_drop.x, active_drop.y)+1.0;
        tmp_pos[2] = active_drop.y;

        if(debug_mode) TeleportPlayers(tmp_pos);
        if(active_drop.disclose) SendMessageToPlayers("<BetterAirdrop> A plane with essential survival supplies is heading towards " + active_drop.name);

        drop_pos[0] = active_drop.x;
        drop_pos[1] = active_drop.y;

        float angle = Math.Atan2(drop_pos[1]-spawn_pos[1], drop_pos[0]-spawn_pos[0]);
        float fixed_angle = angle*Math.RAD2DEG;

        vector direction = drop_pos-spawn_pos;
        float distance = direction.Length();

        vector direction2 = Vector(spawn_pos[0], 0, spawn_pos[1])-Vector(drop_pos[0], 0, drop_pos[1]);
        plane.SetOrientation(Vector(direction2.VectorToAngles()[0], 0, 0)); // thx Dr. Zed (im lazy, i know)

        GetGame().AdminLog("<BetterAirdrop> launch ("+active_drop.name+")"); // Used for CFTools - OmegaSC
        GetGame().AdminLog("<BetterAirdrop> INBOUND " + active_drop.name + " AT [X: "+active_drop.x+"; Y: "+active_drop.y+"] START [X: "+spawn_pos[0]+"; Y: "+spawn_pos[1]+"] DISTANCE2TARGET: "+ distance + "; PLANEANGLE: "+ fixed_angle +";");
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(MovePlane, 20, true);
	}

	void SendMessageToPlayers(string message) {
		ref array<Man> players = new array<Man>;
		GetGame().GetPlayers( players );
		for ( int i = 0; i < players.Count(); i++ )
		{
			PlayerBase player = players.Get(i);
			Param1<string> message_param = new Param1<string>(message);
			GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, message_param, true, player.GetIdentity());
		}
	}

	void TeleportPlayers(vector pos) {
		ref array<Man> players = new array<Man>;
		GetGame().GetPlayers( players );
		for ( int i = 0; i < players.Count(); i++ ) {
			Man player = players.Get(i);
			player.SetPosition(pos);
		}
	}

    bool IsInRect(float x, float y, float min_x, float max_x, float min_y, float max_y) {
        if(x > min_x && x < max_x && y > min_y && y < max_y) return true;
        return false;
    }

    string GetRandomItem() {
        TStringArray loot = {"BookFaustErsterTeil"};
        return loot.GetRandomElement();
    }

	TStringArray WorkingZombieClasses() { // Taken from community offline mode (somewhere is one infected without texture though)
		return {"ZmbM_HermitSkinny_Base","ZmbM_HermitSkinny_Beige","ZmbM_HermitSkinny_Black","ZmbM_HermitSkinny_Green",
				"ZmbM_HermitSkinny_Red","ZmbM_FarmerFat_Base","ZmbM_FarmerFat_Beige","ZmbM_FarmerFat_Blue","ZmbM_FarmerFat_Brown",
				"ZmbM_FarmerFat_Green","ZmbF_CitizenANormal_Base","ZmbF_CitizenANormal_Beige","ZmbF_CitizenANormal_Brown",
				"ZmbF_CitizenANormal_Blue","ZmbM_CitizenASkinny_Base","ZmbM_CitizenASkinny_Blue","ZmbM_CitizenASkinny_Brown",
				"ZmbM_CitizenASkinny_Grey","ZmbM_CitizenASkinny_Red","ZmbM_CitizenBFat_Base","ZmbM_CitizenBFat_Blue","ZmbM_CitizenBFat_Red",
				"ZmbM_CitizenBFat_Green","ZmbF_CitizenBSkinny_Base","ZmbF_CitizenBSkinny","ZmbM_PrisonerSkinny_Base","ZmbM_PrisonerSkinny",
				"ZmbM_FirefighterNormal_Base","ZmbM_FirefighterNormal","ZmbM_FishermanOld_Base","ZmbM_FishermanOld_Blue","ZmbM_FishermanOld_Green",
				"ZmbM_FishermanOld_Grey","ZmbM_FishermanOld_Red","ZmbM_JournalistSkinny_Base","ZmbM_JournalistSkinny","ZmbF_JournalistNormal_Base",
				"ZmbF_JournalistNormal_Blue","ZmbF_JournalistNormal_Green","ZmbF_JournalistNormal_Red","ZmbF_JournalistNormal_White",
				"ZmbM_ParamedicNormal_Base","ZmbM_ParamedicNormal_Blue","ZmbM_ParamedicNormal_Green","ZmbM_ParamedicNormal_Red",
				"ZmbM_ParamedicNormal_Black","ZmbF_ParamedicNormal_Base","ZmbF_ParamedicNormal_Blue","ZmbF_ParamedicNormal_Green",
				"ZmbF_ParamedicNormal_Red","ZmbM_HikerSkinny_Base","ZmbM_HikerSkinny_Blue","ZmbM_HikerSkinny_Green","ZmbM_HikerSkinny_Yellow",
				"ZmbF_HikerSkinny_Base","ZmbF_HikerSkinny_Blue","ZmbF_HikerSkinny_Grey","ZmbF_HikerSkinny_Green","ZmbF_HikerSkinny_Red",
				"ZmbM_HunterOld_Base","ZmbM_HunterOld_Autumn","ZmbM_HunterOld_Spring","ZmbM_HunterOld_Summer","ZmbM_HunterOld_Winter",
				"ZmbF_SurvivorNormal_Base","ZmbF_SurvivorNormal_Blue","ZmbF_SurvivorNormal_Orange","ZmbF_SurvivorNormal_Red",
				"ZmbF_SurvivorNormal_White","ZmbM_SurvivorDean_Base","ZmbM_SurvivorDean_Black","ZmbM_SurvivorDean_Blue","ZmbM_SurvivorDean_Grey",
				"ZmbM_PolicemanFat_Base","ZmbM_PolicemanFat","ZmbF_PoliceWomanNormal_Base","ZmbF_PoliceWomanNormal","ZmbM_PolicemanSpecForce_Base",
				"ZmbM_PolicemanSpecForce","ZmbM_SoldierNormal_Base","ZmbM_SoldierNormal","ZmbM_usSoldier_normal_Base",
				"ZmbM_usSoldier_normal_Woodland","ZmbM_usSoldier_normal_Desert","ZmbM_CommercialPilotOld_Base","ZmbM_CommercialPilotOld_Blue",
				"ZmbM_CommercialPilotOld_Olive","ZmbM_CommercialPilotOld_Brown","ZmbM_CommercialPilotOld_Grey","ZmbM_PatrolNormal_Base",
				"ZmbM_PatrolNormal_PautRev","ZmbM_PatrolNormal_Autumn","ZmbM_PatrolNormal_Flat","ZmbM_PatrolNormal_Summer","ZmbM_JoggerSkinny_Base",
				"ZmbM_JoggerSkinny_Blue","ZmbM_JoggerSkinny_Green","ZmbM_JoggerSkinny_Red","ZmbF_JoggerSkinny_Base","ZmbF_JoggerSkinny_Blue",
				"ZmbF_JoggerSkinny_Brown","ZmbF_JoggerSkinny_Green","ZmbF_JoggerSkinny_Red","ZmbM_MotobikerFat_Base","ZmbM_MotobikerFat_Beige",
				"ZmbM_MotobikerFat_Black","ZmbM_MotobikerFat_Blue","ZmbM_VillagerOld_Base","ZmbM_VillagerOld_Blue","ZmbM_VillagerOld_Green",
				"ZmbM_VillagerOld_White","ZmbM_SkaterYoung_Base","ZmbM_SkaterYoung_Blue","ZmbM_SkaterYoung_Brown","ZmbM_SkaterYoung_Green",
				"ZmbM_SkaterYoung_Grey","ZmbF_SkaterYoung_Base","ZmbF_SkaterYoung_Brown","ZmbF_SkaterYoung_Striped","ZmbF_SkaterYoung_Violet",
				"ZmbF_DoctorSkinny_Base","ZmbF_DoctorSkinny","ZmbF_BlueCollarFat_Base","ZmbF_BlueCollarFat_Blue","ZmbF_BlueCollarFat_Green",
				"ZmbF_BlueCollarFat_Red","ZmbF_BlueCollarFat_White","ZmbF_MechanicNormal_Base","ZmbF_MechanicNormal_Beige","ZmbF_MechanicNormal_Green",
				"ZmbF_MechanicNormal_Grey","ZmbF_MechanicNormal_Orange","ZmbM_MechanicSkinny_Base","ZmbM_MechanicSkinny_Blue","ZmbM_MechanicSkinny_Grey",
				"ZmbM_MechanicSkinny_Green","ZmbM_MechanicSkinny_Red","ZmbM_ConstrWorkerNormal_Base","ZmbM_ConstrWorkerNormal_Beige",
				"ZmbM_ConstrWorkerNormal_Black","ZmbM_ConstrWorkerNormal_Green","ZmbM_ConstrWorkerNormal_Grey","ZmbM_HeavyIndustryWorker_Base",
				"ZmbM_HeavyIndustryWorker","ZmbM_OffshoreWorker_Base","ZmbM_OffshoreWorker_Green","ZmbM_OffshoreWorker_Orange","ZmbM_OffshoreWorker_Red",
				"ZmbM_OffshoreWorker_Yellow","ZmbF_NurseFat_Base","ZmbF_NurseFat","ZmbM_HandymanNormal_Base","ZmbM_HandymanNormal_Beige",
				"ZmbM_HandymanNormal_Blue","ZmbM_HandymanNormal_Green","ZmbM_HandymanNormal_Grey","ZmbM_HandymanNormal_White","ZmbM_DoctorFat_Base",
				"ZmbM_DoctorFat","ZmbM_Jacket_Base","ZmbM_Jacket_beige","ZmbM_Jacket_black","ZmbM_Jacket_blue","ZmbM_Jacket_bluechecks",
				"ZmbM_Jacket_brown","ZmbM_Jacket_greenchecks","ZmbM_Jacket_grey","ZmbM_Jacket_khaki","ZmbM_Jacket_magenta","ZmbM_Jacket_stripes",
				"ZmbF_PatientOld_Base","ZmbF_PatientOld","ZmbM_PatientSkinny_Base","ZmbM_PatientSkinny","ZmbF_ShortSkirt_Base","ZmbF_ShortSkirt_beige",
				"ZmbF_ShortSkirt_black","ZmbF_ShortSkirt_brown","ZmbF_ShortSkirt_green","ZmbF_ShortSkirt_grey","ZmbF_ShortSkirt_checks",
				"ZmbF_ShortSkirt_red","ZmbF_ShortSkirt_stripes","ZmbF_ShortSkirt_white","ZmbF_ShortSkirt_yellow","ZmbF_VillagerOld_Base",
				"ZmbF_VillagerOld_Blue","ZmbF_VillagerOld_Green","ZmbF_VillagerOld_Red","ZmbF_VillagerOld_White","ZmbM_Soldier","ZmbM_SoldierAlice",
				"ZmbM_SoldierHelmet","ZmbM_SoldierVest","ZmbM_SoldierAliceHelmet","ZmbM_SoldierVestHelmet","ZmbF_MilkMaidOld_Base",
				"ZmbF_MilkMaidOld_Beige","ZmbF_MilkMaidOld_Black","ZmbF_MilkMaidOld_Green","ZmbF_MilkMaidOld_Grey","ZmbM_priestPopSkinny_Base",
				"ZmbM_priestPopSkinny","ZmbM_ClerkFat_Base","ZmbM_ClerkFat_Brown","ZmbM_ClerkFat_Grey","ZmbM_ClerkFat_Khaki","ZmbM_ClerkFat_White",
				"ZmbF_Clerk_Normal_Base","ZmbF_Clerk_Normal_Blue","ZmbF_Clerk_Normal_White","ZmbF_Clerk_Normal_Green","ZmbF_Clerk_Normal_Red"};
	}
}