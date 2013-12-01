drop table if exists vmc_details;

create table vmc_details 
(
  vmc_id            int,
  vmc_description   varchar(100),
  vmc_type          varchar(10),  -- VEND or NOTE
  vmc_connection    varchar(10),  -- UDP or MQTT
  vmc_address       varchar(100), -- IP address:port or MQTT topic
  primary key (vmc_id),
  constraint vmc_ref_loc unique (vmc_connection, vmc_address)
) ENGINE = InnoDB; 


insert into vmc_details values (1, 'Studio vending machine', 'VEND', 'UDP' , '192.168.0.12');
insert into vmc_details values (2, 'Note acceptor'         , 'NOTE', 'MQTT', 'nh/note_acc/');
