drop table if exists bells;

create table bells
(
  bell_id           int(11)       not null,
  bell_description  varchar(100)  not null, -- Description of where the bell is / tone / etc
  bell_topic        varchar(100)  not null, -- MQTT topic the bell is subscribed to 
  bell_message      varchar( 50)  not null, -- Payload of the message required to ring the bell
  bell_enabled      boolean       not null default 1, -- enabled yes/no
  unique key (bell_id)
) ENGINE = InnoDB; 


insert into bells values (1, 'Comfy area - 1 ring ', 'nh/gk/bell/ComfyArea', '1', 0); -- IMPLEMENTED IN F/W, DO NOT ENABLE
insert into bells values (2, 'Comfy area - 2 rings', 'nh/gk/bell/ComfyArea', '2', 1);
insert into bells values (3, 'Comfy area - 3 rings', 'nh/gk/bell/ComfyArea', '3', 1);

insert into bells values (4, 'Workshop - 1 ring ', 'nh/gk/bell/Workshop', '1', 1);
insert into bells values (5, 'Workshop - 2 rings', 'nh/gk/bell/Workshop', '2', 1);
insert into bells values (6, 'Workshop - 3 rings', 'nh/gk/bell/Workshop', '3', 1);
