drop table if exists grp;

create table grp 
(
  grp_id          int not null auto_increment,
  grp_description varchar(200),
  primary key (grp_id),
  constraint grp_desc unique (grp_description)
) ENGINE = InnoDB; 

insert into grp (grp_id, grp_description) values (1, 'Full Access');