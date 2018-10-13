drop procedure if exists sp_temperature_update;

DELIMITER //
CREATE PROCEDURE sp_temperature_update
(
  IN address   varchar(16),
  IN temp_update  float
)
SQL SECURITY DEFINER
BEGIN
  declare address_exists int;
  set address_exists = 0;
  
  SELECT count(*) into address_exists
  FROM temperature t
  WHERE t.dallas_address = address;
  
  if (address_exists = 1) then
    UPDATE temperature
    SET 
      temperature = (temp_update),
      time = now()
    WHERE dallas_address = address;
  else
    INSERT INTO temperature (dallas_address, temperature)
    VALUES (address, temp_update);
  end if;

END //
DELIMITER ;

