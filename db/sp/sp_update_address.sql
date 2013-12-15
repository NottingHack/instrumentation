drop procedure if exists sp_update_address;

DELIMITER //
CREATE PROCEDURE sp_update_address
(
   IN p_address   varchar(100),
   IN p_last_seen timestamp
)
SQL SECURITY DEFINER
BEGIN
  insert into addresses (mac_address,last_seen) values (p_address, p_last_seen) on duplicate key update last_seen = p_last_seen;
END //
DELIMITER ;

