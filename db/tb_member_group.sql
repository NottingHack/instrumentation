drop table if exists member_group;

create table member_group 
(
  member_id int not null,
  grp_id int not null,
  primary key (member_id, grp_id)
) ENGINE = InnoDB; 


