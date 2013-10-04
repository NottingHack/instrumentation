drop procedure if exists sp_temperature_get_desc;

DELIMITER //
CREATE PROCEDURE sp_temperature_get_desc
(
  IN  address varchar(16),
  OUT name    varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare address_exists int;
  set address_exists = 0;
  
  select t.name
  into name
  from temperature t
  where t.dallas_address = address
    and t.name is not null;

END //
DELIMITER ;
