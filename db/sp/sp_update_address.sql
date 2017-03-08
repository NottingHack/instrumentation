drop procedure if exists sp_update_address;

DELIMITER //
CREATE PROCEDURE sp_update_address
(
   IN p_address   varchar(100),
   IN p_last_seen timestamp
)
SQL SECURITY DEFINER
BEGIN

  -- Nb: did use an insert with "on duplicate key update last_seen = p_last_seen" to avoid the below, but
  --     when chagning to InnoDB to caused the auto incremeting ID to go crazy
  if exists (select null from addresses where mac_address = p_address) then
    update addresses set last_seen = p_last_seen where mac_address = p_address;
  else
    insert into addresses (mac_address,last_seen) values (p_address, p_last_seen);
  end if;
END //
DELIMITER ;

