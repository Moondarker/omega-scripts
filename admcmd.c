  /*
    Place in init.c  
  */
  
  bool verify_admins = false; // true=verify presence of BI UID in admin list
	string cmd_prefix = "/"; // Must be special character
	ref TStringArray admins = {}; // Add your BI UID

	bool IsPlayerAnAdmin(PlayerBase player) {
		bool found = false;
		for ( int i = 0; i < admins.Count(); ++i ) {
			if(player.GetIdentity().GetId() == admins[i]) { found=true; break; }
		}
		return found;
	}

	void SendMessageToPlayer(PlayerBase player, string message) {
		Param1<string> param = new Param1<string>( message );
		GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, param, true, player.GetIdentity());
	}

	bool IsPlayer(string name) {
		PlayerBase p;
		array<Man> players = new array<Man>; GetGame().GetPlayers(players);
		for ( int i = 0; i < players.Count(); ++i ) {
			p = players.Get(i);
			if(p.GetIdentity().GetName() == name) return true;
		}
		return false;
	}

	PlayerBase GetPlayer(string name) {
		PlayerBase p;
		array<Man> players = new array<Man>; GetGame().GetPlayers(players);
		for ( int i = 0; i < players.Count(); ++i ) {
			p = players.Get(i);
			if(p.GetIdentity().GetName() == name) return p;
		}
		return NULL;
	}

	override void OnEvent(EventType eventTypeId, Param params)  {
		super.OnEvent(eventTypeId,params);
		int i;
		PlayerBase player, temp_player;
		array<Man> players = new array<Man>; GetGame().GetPlayers(players);
		if(eventTypeId != ChatMessageEventTypeID) return; // Is chat message
		ChatMessageEventParams chat_params = ChatMessageEventParams.Cast( params );
		if(chat_params.param1 != 0 || chat_params.param2 == "") return; 
		if(!IsPlayer(chat_params.param2)) return;
		player = GetPlayer(chat_params.param2);
		if(verify_admins && !IsPlayerAnAdmin(player)) { GetGame().AdminLog("[ADMCMD] (Unauthorized) " + player.GetIdentity().GetName() +" ("+player.GetIdentity().GetPlainId()+", "+player.GetIdentity().GetId()+") tried to execute "+ chat_params.param3); return; }
		string message = chat_params.param3, prefix, param0, command;
		TStringArray tokens = new TStringArray;
		message.Split(" ", tokens); int count = tokens.Count();
		param0 = tokens.Get(0);
		param0.ParseStringEx(prefix); if(prefix != cmd_prefix) return;
		param0.ParseStringEx(command);
		GetGame().AdminLog("[ADMCMD] PLAYER: "+ player.GetIdentity().GetName() +" ("+player.GetIdentity().GetPlainId()+", "+player.GetIdentity().GetId()+") CMD: "+ command);
		switch(command) {
			case "spawn": {
				if(count != 2) { SendMessageToPlayer(player, "/spawn [object]"); return; }
				GetGame().CreateObject(tokens[1], player.GetPosition(), false, true );
				SendMessageToPlayer(player, "[ObjectSpawn] Object spawned: " + tokens[1]);
				break;
			}

			case "goto": {
				if(count != 2) { SendMessageToPlayer(player, "/goto [player]"); return; }
				if(!IsPlayer(tokens[1])) {
					SendMessageToPlayer(player, "[Teleport] Can't find player called: '"+tokens[1]+"'");
				} else {
					temp_player = GetPlayer(tokens[1]);
					player.SetPosition(temp_player.GetPosition());
					SendMessageToPlayer(player, "[Teleport] You teleported to " + temp_player.GetIdentity().GetName());
				}
				break;
			}

			case "allgoto": {
				PlayerBase allgoto_target;
				if(count != 2) { SendMessageToPlayer(player, "/allgoto [player]"); return; }
				if(!IsPlayer(tokens[1])) {
					SendMessageToPlayer(player, "[Teleport] Can't find player called: '"+tokens[1]+"'");
				} else {
					allgoto_target = GetPlayer(tokens[1]);
					SendMessageToPlayer(player, "[Teleport] You teleported everyone to your location");
					for (i = 0; i < players.Count(); i++) {
						temp_player = players.Get(i);
						SendMessageToPlayer(temp_player, "[Teleport] You have been teleported to player " + allgoto_target.GetIdentity().GetName());
						temp_player.SetPosition(allgoto_target.GetPosition());
					}
				}
				break;
			}

			case "here": {
				if(count != 2) { SendMessageToPlayer(player, "/here [player]"); return; }
				if(!IsPlayer(tokens[1])) {
					SendMessageToPlayer(player, "[Teleport] Can't find player called: '"+tokens[1]+"'");
				} else {
					temp_player = GetPlayer(tokens[1]);
					temp_player.SetPosition(player.GetPosition());
					SendMessageToPlayer(temp_player, "[Teleport] You have been teleported to admin " + player.GetIdentity().GetName());
					SendMessageToPlayer(player, "[Teleport] You teleported " + temp_player.GetIdentity().GetName() + " to your location");
				}
				break;
			}

			case "allhere": {
				SendMessageToPlayer(player, "[Teleport] You teleported everyone to your location");
				for (i = 0; i < players.Count(); i++) {
					temp_player = players.Get(i);
					SendMessageToPlayer(temp_player, "[Teleport] You have been teleported to admin " + player.GetIdentity().GetName());
					temp_player.SetPosition(player.GetPosition());
				}
				break;
			}

			case "time": {
				if(count != 3) { SendMessageToPlayer(player, "/time [hour] [minute]"); return; }
				GetGame().GetWorld().SetDate( 2018, 1, 7, tokens[1].ToInt(), tokens[2].ToInt());
				SendMessageToPlayer(player, "[Servertime] You have set the servertime to " + tokens[1] + ":"+tokens[2]);
				break;
			}

			case "kill": {
				if(count == 2) {
					if(!IsPlayer(tokens[1])) {
						SendMessageToPlayer(player, "[Kill] Can't find player called: '"+tokens[1]+"'");
					} else {
						temp_player = GetPlayer(tokens[1]);
						temp_player.SetHealth(0);
						SendMessageToPlayer(player, "[Kill] You killed " + temp_player.GetIdentity().GetName());
					}
				} else {
					player.SetHealth(0);
					SendMessageToPlayer(player, "[Kill] You killed yourself");
				}
				break;
			}

			case "killall": {
				SendMessageToPlayer(player, "[Kill] You killed everyone");
				for (i = 0; i < players.Count(); i++) {
					temp_player = players.Get(i);
					if(temp_player.GetIdentity().GetId() == player.GetIdentity().GetId()) continue;
					temp_player.SetHealth(0);
				}
				break;
			}

			case "heal": {
				PlayerBase heal_target;
				if(count == 2) {
					if(!IsPlayer(tokens[1])) {
						SendMessageToPlayer(player, "[Heal] Can't find player called: '"+tokens[1]+"'");
					} else {
						heal_target = GetPlayer(tokens[1]);
						SendMessageToPlayer(player, "[Heal] You healed " + heal_target.GetIdentity().GetName());
					}
				} else {
					heal_target = player;
					SendMessageToPlayer(player, "[Heal] You healed yourself");
				}
				if(heal_target != NULL) {
					heal_target.SetHealth(heal_target.GetMaxHealth("", ""));
					heal_target.SetHealth("", "Blood", heal_target.GetMaxHealth("", "Blood"));
					heal_target.GetStatStamina().Set(1000);
					heal_target.GetStatEnergy().Set(1000);
					heal_target.GetStatWater().Set(1000);
				}
				break;
			}

			case "offroad": {
				SendMessageToPlayer(player, "[Offroad] Vehicled spawned");
        EntityAI v;
        v = GetGame().CreateObject( "OffroadHatchback", player.GetPosition() + "1.5 0 1.5");		    
        v.GetInventory().CreateAttachment("SparkPlug");
        v.GetInventory().CreateAttachment("EngineBelt");
        v.GetInventory().CreateAttachment("CarBattery");
        v.GetInventory().CreateAttachment("HatchbackHood");
        v.GetInventory().CreateAttachment("HatchbackTrunk");
        v.GetInventory().CreateAttachment("HatchbackDoors_CoDriver");
        v.GetInventory().CreateAttachment("HatchbackWheel");
        v.GetInventory().CreateAttachment("HatchbackWheel");
        v.GetInventory().CreateAttachment("HatchbackWheel");
        v.GetInventory().CreateAttachment("HatchbackWheel");
        v.GetInventory().CreateAttachment("HatchbackWheel"); // spare
				break;
			}
			
			default: {
				SendMessageToPlayer(player, "Unknown command: " + command);
				break;
			}
		}
	}
