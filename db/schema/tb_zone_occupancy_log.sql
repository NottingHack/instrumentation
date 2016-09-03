
drop table if exists zone_occupancy_log;

create table zone_occupancy_log
(
  zone_occ_log_id int not null auto_increment,
  zone_id         int not null,
  member_id       int not null,
  time_exited     timestamp default CURRENT_TIMESTAMP,
  time_entered    timestamp,
  primary key (zone_occ_log_id)
) ENGINE = InnoDB; 
