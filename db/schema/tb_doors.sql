drop table if exists doors;

create table doors
(
  door_id           int(11) not null,
  door_description  varchar(100),
  door_short_name   varchar(10),
  door_state        varchar(10),
  door_state_change datetime,
  unique key (door_id)
) ENGINE = InnoDB; 


insert into doors values (1, 'Upstairs inner Gatekeeper door', 'UP-INNER', NULL, NULL);
insert into doors values (2, 'Upstairs outer door'           , 'UP-OUTER', NULL, NULL);
