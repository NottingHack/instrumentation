drop table if exists group_permissions;

create table group_permissions 
(
  grp_id int not null,
  permission_code varchar(16) not null,
  primary key (grp_id, permission_code)
) ENGINE = InnoDB; 

insert into group_permissions 
select 1, permission_code 
from permissions;