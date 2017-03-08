drop table if exists door_bells;

/* Specify which door bell buttons ring which bells */


create table door_bells
(
  door_id int(11) not null,
  bell_id int(11) not null,
  primary key (door_id, bell_id)
) ENGINE = InnoDB; 


-- Inner door bell button rings the comfy area & workshop bells with 1 ring
insert into door_bells values (1, 1);
insert into door_bells values (1, 4);

-- Outer door bell button rings the comfy area & workshop bells with 2 rings
insert into door_bells values (2, 2);
insert into door_bells values (2, 5);

-- Workshop door bell button (which doesn't exist yet) rings the workshop bell with 3 rings
insert into door_bells values (3, 6);

