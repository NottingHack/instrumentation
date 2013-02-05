drop procedure if exists sp_gg_get_address_id;

/* Return address_id for given email address, adding 
 * to gg_addresses if required */

DELIMITER //
CREATE PROCEDURE sp_gg_get_address_id
(
  IN  ggemail_from   varchar(200),
  OUT ggaddresses_id int
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  
  select count(*) 
  into ck_exists
  from gg_addresses gga
  where gga.ggaddress_email = ggemail_from;
  
  if (ck_exists > 0) then
    select gga.ggaddresses_id 
    into ggaddresses_id
    from gg_addresses gga
    where gga.ggaddress_email = ggemail_from;
  else
    insert into gg_addresses (ggaddress_email)
    values (ggemail_from);
    
    set ggaddresses_id = last_insert_id();
  end if;  

END //
DELIMITER ;


GRANT EXECUTE ON PROCEDURE sp_gg_get_address_id TO 'gk'@'localhost'