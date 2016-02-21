drop table if exists doors;

create table doors
(
  door_id           int(11)      not null, -- Door id. Note: this MUST match the ID in the appropriate gatekeeper firmware
  door_description  varchar(100) not null, -- Full description of where the door is
  door_short_name   varchar(16)  not null, -- Short description used for slack/irc/messages boards on doorbell/door opened event
  door_state        varchar(10)  not null, -- OPEN, CLOSED or UNKNOWN
  door_state_change datetime     not null, -- Time the door_state changed
  permission_code   varchar(16)  null,     -- link to permissions.permission_code to restict access to door (NULL=unrestircted)
  side_a_zone_id    int(11)      null,     -- Zone id of the zone on side a of the door (zone 0 = outside/off-site)
  side_b_zone_id    int(11)      null,     -- Zone id of the zone on side b of the door 
  unique key (door_id)
) ENGINE = InnoDB;



-- if only one side has a zone set, it means the door is push to exit is the other direction
insert into doors values (1, 'Upstairs inner Gatekeeper door', 'UP-INNER'     , 'UNKNOWN', sysdate(), NULL,    0, NULL);
insert into doors values (2, 'Upstairs outer door'           , 'UP-OUTER'     , 'UNKNOWN', sysdate(), NULL, NULL, NULL); -- Not RFID controlled
insert into doors values (3, 'Workshop door'                 , 'WORKSHOP'     , 'UNKNOWN', sysdate(), NULL, NULL, NULL); -- Not RFID controlled
insert into doors values (4, 'Goods lift entrance'           , 'GOODS'        , 'UNKNOWN', sysdate(), NULL,    0,    2);
insert into doors values (5, 'Team storage'                  , 'TEAMSTORE'    , 'UNKNOWN', sysdate(), NULL,    2, NULL);
insert into doors values (6, 'Communal (Left)'               , 'COMMUNAL-L'   , 'UNKNOWN', sysdate(), NULL,    0,    2);
insert into doors values (7, 'Communal (Right)'              , 'COMMUNAL-R'   , 'UNKNOWN', sysdate(), NULL,    0,    1);
insert into doors values (8, 'Members storage'               , 'MEMBERSSTORE' , 'UNKNOWN', sysdate(), NULL,    1, NULL);
insert into doors values (9, 'Alfred Street South entrance'  , 'STREET'       , 'UNKNOWN', sysdate(), NULL,    0,    1);
