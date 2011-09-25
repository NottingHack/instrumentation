drop procedure if exists sp_space_net_activity;

DELIMITER //
CREATE PROCEDURE sp_space_net_activity
(
   OUT mesg varchar(256)
)
SQL SECURITY DEFINER
BEGIN

  declare addr_count int;
  declare addr_last_time int;
  declare hs_state varchar(25);

  set mesg = '';
  
  select count(*) 
  into addr_count
  from addresses 
  where (timestampdiff(SECOND, last_seen, sysdate()) < 300) 
    and ignore_addr=0;

  select timestampdiff(SECOND, last_seen, sysdate()) 
  into addr_last_time
  from addresses 
  where ignore_addr=0
  order by last_seen desc
  limit 1;
 
  select event_type
  into hs_state
  from events
  where event_type in ('FIRST_IN', 'LAST_OUT')
  order by event_time desc, event_id desc
  limit 1; 
 
  if (hs_state = 'FIRST_IN') then
    set mesg = concat(mesg, 'Last man out switch: Open, ');
  else
    set mesg = concat(mesg, 'Last man out switch: Closed, ');
  end if;
    

  set mesg = concat(mesg, addr_count, ' addresses seen in the last 5 minutes (most recent ', addr_last_time, 's ago)\n');


END //
DELIMITER ;

GRANT EXECUTE ON PROCEDURE sp_space_net_activity TO 'gk'@'localhost'