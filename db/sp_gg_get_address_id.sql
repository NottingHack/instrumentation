drop procedure if exists sp_gg_get_address_id;

/* Return address_id for given email "From: " string, adding 
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
  declare email_address varchar(200);
  declare email_name varchar(200);
  
  -- First step: extract email address. Possibilities:
  -- From: Member Name <member@domain.com>
  -- From: "Member Name" <member@domain.com>
  -- From: member@domain.com

  if ((instr(ggemail_from, '<') = 0) or (instr(ggemail_from, '>') = 0)) then
    set email_address = ggemail_from;
    set email_name = '';
  else
    set email_address = substr(ggemail_from, 
                        instr(ggemail_from, '<')+1, 
                        instr(ggemail_from, '>') - instr(ggemail_from, '<')-1);
    set email_name = substr(ggemail_from, 1, instr(ggemail_from, '<')-2);
  
    -- Remove quotes if name is quoted
    set email_name = replace(email_name, '"', '');
  end if;
  
  -- if something's gone wrong, use string as passed in
  if (email_address is null) then
    set email_address = ggemail_from;
  end if;
  
  if (email_name is null) then
    set email_name = '';
  end if;
  
  select count(*) 
  into ck_exists
  from gg_addresses gga
  where gga.ggaddress_email = email_address;
  
  if (ck_exists > 0) then
    select gga.ggaddresses_id 
    into ggaddresses_id
    from gg_addresses gga
    where gga.ggaddress_email = email_address;
  else
    insert into gg_addresses (ggaddress_email, ggaddress_name)
    values (email_address, email_name);
    
    set ggaddresses_id = last_insert_id();
  end if;  
  
END //
DELIMITER ;
