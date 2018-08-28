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

insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'A1', '00-00');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'A2', '00-01');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'A3', '00-02');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'A4', '00-03');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'A5', '00-04');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'A6', '00-05');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'A7', '00-06');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'A8', '00-07');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'A9', '00-08');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'B1', '00-0a');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'B2', '00-0b');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'B3', '00-0c');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'B4', '00-0d');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'B5', '00-0e');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'B6', '00-0f');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'B7', '00-10');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'B8', '00-11');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'B9', '00-12');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'C1', '00-14');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'C2', '00-15');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'C3', '00-16');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'C4', '00-17');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'C5', '00-18');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'C6', '00-19');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'C7', '00-1a');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'C8', '00-1b');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'C9', '00-1c');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'D1', '00-1e');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'D2', '00-1f');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'D3', '00-20');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'D4', '00-21');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'D5', '00-22');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'D6', '00-23');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'D7', '00-24');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'D8', '00-25');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'D9', '00-26');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'E1', '00-28');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'E2', '00-29');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'E3', '00-2a');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'E4', '00-2b');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'E5', '00-2c');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'E6', '00-2d');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'E7', '00-2e');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'E8', '00-2f');
insert into vmc_ref (vmc_id, loc_name, loc_encoded) values (4, 'E8', '00-30');
