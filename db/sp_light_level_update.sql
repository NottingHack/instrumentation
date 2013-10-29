drop procedure if exists sp_light_level_update;

DELIMITER //
CREATE PROCEDURE sp_light_level_update
(
  IN sensor       varchar(30),
  IN light_level  int
)
SQL SECURITY DEFINER
BEGIN
  declare sensor_exists int;
  set sensor_exists = 0;
  
  select count(*) into sensor_exists
  from light_level l
  where l.sensor = sensor;
  
  if (sensor_exists = 1) then
    update light_level
    set reading = light_level
    where light_level.sensor = sensor;
  else
    insert into light_level (sensor, reading)
    values (sensor, light_level);
  end if;

END //
DELIMITER ;

