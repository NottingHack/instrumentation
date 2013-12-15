drop procedure if exists sp_space_net_activity;

DELIMITER //
CREATE PROCEDURE sp_space_net_activity
(
   OUT mesg varchar(256)
)
SQL SECURITY DEFINER
BEGIN

  declare addr_count int;
  declare addr_last_time_m int;
  declare addr_last_time_s int;
  declare hs_state varchar(25);
  declare door_open int;
  declare door_state varchar(25);
  declare door_event_time_m int;
  declare door_event_time_s int;

  set mesg = '';
  
  -- Assume door closed to start with
  set door_open = 0; 
  
  select count(*) 
  into addr_count
  from addresses 
  where (timestampdiff(SECOND, last_seen, sysdate()) < 600) 
    and ignore_addr=0;

  select timestampdiff(MINUTE, last_seen, sysdate()), timestampdiff(SECOND, last_seen, sysdate()) 
  into addr_last_time_m, addr_last_time_s
  from addresses 
  where ignore_addr=0
  order by last_seen desc
  limit 1;
 
  -- Get the current state of the LMS
  select event_type
  into hs_state
  from events
  where event_type in ('FIRST_IN', 'LAST_OUT')
  order by event_time desc, event_id desc
  limit 1; 
 
  -- See if the door is open or closed
  select 
    event_type, 
    timestampdiff(MINUTE, event_time, sysdate()), 
    timestampdiff(SECOND, event_time, sysdate()) 
  into 
    door_state, 
    door_event_time_m,
    door_event_time_s
  from events
  where event_type in ('DOOR_OPENED', 'DOOR_CLOSED')
  order by event_time desc, event_id desc
  limit 1; 
 
  -- LMS text
  if (hs_state = 'FIRST_IN') then
    set mesg = concat(mesg, 'Last man out switch: Open, ');
  else
    set mesg = concat(mesg, 'Last man out switch: Closed, ');
  end if;
    
  -- Adddress count text
  if (addr_last_time_s is not null) then
    if (addr_last_time_s < 60) then
      set mesg = concat(mesg, addr_count, ' network devices seen in the last 10 minutes (most recent less than a minute ago). ');
    else
      set mesg = concat(mesg, addr_count, ' network devices seen in the last 10 minutes (most recent ', addr_last_time_m, 'm ago). ');
    end if;
  end if;
  
  -- Door state text
  if (door_event_time_m is not null) then
    if (door_event_time_m >= 2) then
      set mesg = concat(mesg, 'The door was last open ', door_event_time_m, 'm ago');
    else
      set mesg = concat(mesg, 'The door was last open ', door_event_time_s, 's ago');
    end if;
    
    if (door_state = 'DOOR_OPENED') then
      set mesg = concat(mesg, ' (and is open NOW).');
    else
      set mesg = concat(mesg, '.');
    end if;    
  end if;
  


END //
DELIMITER ;

