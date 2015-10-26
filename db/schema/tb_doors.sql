drop table if exists doors;

create table doors
(
  door_id           int(11)      not null, -- Door id. Note: this MUST match the ID in the appropriate gatekeeper firmware
  door_description  varchar(100) not null, -- Full description of where the door is
  door_short_name   varchar(10)  not null, -- Short description used for slack/irc/messages boards on doorbell/door opened event
  door_state        varchar(10)  not null, -- OPEN, CLOSED or UNKNOWN
  door_state_change datetime     not null, -- Time the door_state changed
  permission_code   varchar(16)  null,     -- link to permissions.permission_code to restict access to door (NULL=unrestircted)
  unique key (door_id)
) ENGINE = InnoDB; 


insert into doors values (1, 'Upstairs inner Gatekeeper door', 'UP-INNER', 'UNKNOWN', sysdate(), NULL);
insert into doors values (2, 'Upstairs outer door'           , 'UP-OUTER', 'UNKNOWN', sysdate(), NULL);
insert into doors values (3, 'Test resticted door'           , 'R1'      , 'UNKNOWN', sysdate(), 'RSTR1');
