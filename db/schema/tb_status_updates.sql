drop table if exists status_updates;

create table status_updates 
(
  id int not null auto_increment,
  member_id int not null,
  admin_id int not null,
  old_status int not null,
  new_status int not null,
  timestamp timestamp not null default CURRENT_TIMESTAMP,
  primary key (id)
) ENGINE=InnoDB;
