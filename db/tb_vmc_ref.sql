drop table if exists vmc_ref;

create table vmc_ref 
(
  vmc_ref_id    int not null auto_increment,
  vmc_id        int, -- vending machine id - always 1 for now.
  loc_encoded   varchar(10),
  loc_name      varchar(10),
  primary key (vmc_ref_id),
  constraint vmc_ref_loc unique (vmc_id, loc_encoded)
) ENGINE = InnoDB; 


insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'A1', '41-31');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'A3', '41-33');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'A5', '41-35');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'A7', '41-37');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'B1', '42-31');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'B3', '42-33');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'B5', '42-35');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'B7', '42-37');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'C1', '43-31');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'C3', '43-33');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'C5', '43-35');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'C7', '43-37');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'D1', '44-31');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'D3', '44-33');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'D5', '44-35');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'D7', '44-37');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'E1', '45-31');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'E2', '45-32');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'E3', '45-33');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'E4', '45-34');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'E5', '45-35');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'E6', '45-36');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'E7', '45-37');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'E8', '45-38');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'F1', '46-31');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'F2', '46-32');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'F3', '46-33');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'F4', '46-34');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'F5', '46-35');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'F6', '46-36');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (1, 'F7', '46-37');
