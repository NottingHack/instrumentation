drop table if exists zones;

create table zones
(
  zone_id           int(11)      not null, -- Zone id. links to doors.side_a_zone_id & doors.side_b_zone_id
  zone_description  varchar(100) not null, -- Long/full description of zone
  zone_short_name   varchar(10)  not null, -- Short description of zone
  permission_code   varchar(16)  null,     -- Permision code members must have to enter the zone (or null for open access)
  primary key (zone_id)
) ENGINE = InnoDB;  

   insert into zones values (0, 'Off-site'                              , 'Off-site',  NULL  ); -- n.b. zone_id=0 is "specical"
   insert into zones values (1, 'Metalworking, classroom & bike storage', 'Zone 1'  , 'ZONE1');
-- insert into zones values (2, 'Members storage'                       , 'Zone 2'  , 'ZONE2');
   insert into zones values (2, 'Laser, CNC & blue room'                , 'Zone 2'  , 'ZONE2');
-- insert into zones values (4, 'Infastructure & team storage'          , 'Zone 4'  , 'ZONE4');
