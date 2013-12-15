drop procedure if exists sp_pin_insert;

/*
 * Create PIN entry for member_id
 * Only one pin_id can have the same enroll/active PIN at the same time

*/

DELIMITER //
CREATE PROCEDURE sp_pin_insert 
(
   IN   pin       varchar(12),
   IN   expiry    varchar(20),
   IN   state     int,
   IN   member_id int,
   OUT  err       varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  declare expi timestamp;
  set ck_exists = 0;
 
  main: begin  
    set err = '';
    
    -- Don't allow multiple copies of the same PIN to be active
    select count(*) into ck_exists
    from pins p
    where p.pin = pin
      and p.state in (10, 40); -- active, enroll
      
    if (ck_exists > 0) then
      set err = "PIN already in use";
      leave main;
    end if;
    
    -- Check state is valid
    if (state not in (10, 20, 30, 40)) then
      set err = "Invalid state";
      leave main;
    end if;
    
    -- Check member_id is valid
    select count(*) into ck_exists
    from members m
    where m.member_id = member_id;
      
    if (ck_exists != 1) then
      set err = "Unable to find member";
      leave main;
    end if;    
  
    if (expiry = '') then
      set expi = NULL;
    else
      set expi = str_to_date(expiry, '%d/%m/%Y %T');
    end if;
  
    insert into pins (pin, unlock_text, expiry, state, member_id)
    values (pin, 'N/A', expi, state, member_id);

  end main;
  

END //
DELIMITER ;

