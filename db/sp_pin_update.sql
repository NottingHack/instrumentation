drop procedure if exists sp_pin_update;

/*
 * Update the state/expiry of a PIN by pin_id.
 * Only one pin_id can have the same enroll/active PIN at the same time

*/

DELIMITER //
CREATE PROCEDURE sp_pin_update 
(
   IN   pin_id  int,
   IN   expiry  varchar(20),
   IN   state   int,
   OUT  err     varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  set ck_exists = 0;
 
  main: begin  
    set err = '';
    
    -- Don't allow multiple copies of the same PIN to be active
    select count(*) into ck_exists
    from pins p
    where p.pin = (select p2.pin from pins p2 where p2.pin_id = pin_id)
      and p.state in (10, 40) -- active, enroll
      and p.pin_id != pin_id;
      
    if (ck_exists > 0) then
      set err = "PIN already in use";
      leave main;
    end if;

    update pins
    set 
      pins.expiry = 
        case 
          when expiry = '' then null
          else str_to_date(expiry, '%d/%m/%Y %T')
         end,
      pins.state = state
    where
      pins.pin_id = pin_id;    
    
  end main;
  

END //
DELIMITER ;
