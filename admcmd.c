  /*
    Place in init.c -> CustomMission
  */
  
  	bool verify_admins = false; // true=verify presence of BI UID in admin list
	string cmd_prefix = "/"; // Must be special character
	ref TStringArray admins = {}; // Add your BI UID or SteamID

	bool IsPlayerAnAdmin(PlayerBase player) {
		bool found = false;
		for ( int i = 0; i < admins.Count(); ++i ) {
			if(player.GetIdentity().GetId() == admins[i] || player.GetIdentity().GetPlainId() == admins[i]) { found=true; break; }
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
		player = GetPlayer(chat_params.param2);
		if(player == NULL) return;
		if(verify_admins && !IsPlayerAnAdmin(player)) { GetGame().AdminLog("[ADMCMD] (Unauthorized) " + player.GetIdentity().GetName() +" ("+player.GetIdentity().GetPlainId()+", "+player.GetIdentity().GetId()+") tried to execute "+ chat_params.param3); return; }
		string message = chat_params.param3, prefix, param0, command;
		TStringArray tokens = new TStringArray;
		message.Replace("` ", "&SPCESC!"); //Escape spaces before splitting
		message.Split(" ", tokens); int count = tokens.Count();

		for ( i = 0; i < count; ++i ) {
			message = tokens[i];
			message.Replace("&SPCESC!", " "); //.Replace doesn't work directly on TStringArray elements for some reason :(
			tokens[i] = message;
		}

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

			case "topos": {
				if (count < 3) { SendMessageToPlayer(player, "/topos [x] [y] (player)"); return; }
				float ATL_Z = GetGame().SurfaceY(tokens[1].ToFloat(), tokens[2].ToFloat());
				vector reqpos = Vector(tokens[1].ToFloat(), ATL_Z, tokens[2].ToFloat());
				temp_player = player;
				if (count == 4) { 
					temp_player = GetPlayer(tokens[3]); 
					if (temp_player == NULL) {
						SendMessageToPlayer(player, "[Teleport] Can't find player called: '"+tokens[3]+"'"); return;
					} else {
						SendMessageToPlayer(temp_player, "[Teleport] You've been teleported to " + reqpos + " by admin " + player.GetIdentity().GetName());
					}
				}
				temp_player.SetPosition(reqpos);
				SendMessageToPlayer(player, "[Teleport] Target teleported to " + temp_player.GetPosition());
				break;
			}

			case "goto": {
				if(count != 2) { SendMessageToPlayer(player, "/goto [player]"); return; }
				temp_player = GetPlayer(tokens[1]);
				if(temp_player == NULL) {
					SendMessageToPlayer(player, "[Teleport] Can't find player called: '"+tokens[1]+"'");
				} else {
					player.SetPosition(temp_player.GetPosition());
					SendMessageToPlayer(player, "[Teleport] You teleported to " + temp_player.GetIdentity().GetName());
				}
				break;
			}

			case "allgoto": {
				PlayerBase allgoto_target;
				if(count != 2) { SendMessageToPlayer(player, "/allgoto [player]"); return; }
				allgoto_target = GetPlayer(tokens[1]);
				if(allgoto_target == NULL) {
					SendMessageToPlayer(player, "[Teleport] Can't find player called: '"+tokens[1]+"'");
				} else {
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
				temp_player = GetPlayer(tokens[1]);
				if(temp_player == NULL) {
					SendMessageToPlayer(player, "[Teleport] Can't find player called: '"+tokens[1]+"'");
				} else {
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
					temp_player = GetPlayer(tokens[1]);
					if(temp_player == NULL) {
						SendMessageToPlayer(player, "[Kill] Can't find player called: '"+tokens[1]+"'");
					} else {
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
					heal_target = GetPlayer(tokens[1]);
					if(heal_target == NULL) {
						SendMessageToPlayer(player, "[Heal] Can't find player called: '"+tokens[1]+"'");
					} else {
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
				Car v;
				float playerAngle = MiscGameplayFunctions.GetHeadingAngle(player);
				vector posModifier = Vector(-(3 * Math.Sin(playerAngle)), 0, 3 * Math.Cos(playerAngle));
				v = Car.Cast(GetGame().CreateObject( "OffroadHatchback", player.GetPosition() + posModifier));
				
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

			case "refuel": {
				ref array<Object> nearest_objects = new array<Object>;
				ref array<CargoBase> proxy_cargos = new array<CargoBase>;
				Car toBeFilled;
				vector position = player.GetPosition();
				GetGame().GetObjectsAtPosition ( position, 10, nearest_objects, proxy_cargos );
		
				for (i = 0; i < nearest_objects.Count(); i++) {
					if (nearest_objects[i].IsKindOf("CarScript")) {
						SendMessageToPlayer(player, "[Refuel] Found car: '"+nearest_objects[i]+"'");
						toBeFilled = Car.Cast(nearest_objects[i]);
						float fuelReq = toBeFilled.GetFluidCapacity( CarFluid.FUEL ) - (toBeFilled.GetFluidCapacity( CarFluid.FUEL ) * toBeFilled.GetFluidFraction( CarFluid.FUEL ));
						float oilReq = toBeFilled.GetFluidCapacity( CarFluid.OIL ) - (toBeFilled.GetFluidCapacity( CarFluid.OIL ) * toBeFilled.GetFluidFraction( CarFluid.OIL ));
						float coolantReq = toBeFilled.GetFluidCapacity( CarFluid.COOLANT ) - (toBeFilled.GetFluidCapacity( CarFluid.COOLANT ) * toBeFilled.GetFluidFraction( CarFluid.COOLANT ));
						float brakeReq = toBeFilled.GetFluidCapacity( CarFluid.BRAKE ) - (toBeFilled.GetFluidCapacity( CarFluid.BRAKE ) * toBeFilled.GetFluidFraction( CarFluid.BRAKE ));
						toBeFilled.Fill( CarFluid.FUEL, fuelReq );
						toBeFilled.Fill( CarFluid.OIL, oilReq );
						toBeFilled.Fill( CarFluid.COOLANT, coolantReq );
						toBeFilled.Fill( CarFluid.BRAKE, brakeReq );
						SendMessageToPlayer(player, "[Refuel] "+fuelReq+"L added, all fluids maxed");
					}
				}
				
				break;
			}

			default: {
				SendMessageToPlayer(player, "Unknown command: " + command);
				break;
			}
		}
	}
