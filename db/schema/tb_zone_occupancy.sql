
drop table if exists zone_occupancy;

create table zone_occupancy 
(
  zone_id       int not null,
  member_id     int not null,
  time_entered  timestamp default CURRENT_TIMESTAMP,
  primary key (member_id)
) ENGINE = InnoDB; 
