drop procedure if exists sp_get_address_stats;

DELIMITER //
CREATE PROCEDURE sp_get_address_stats
(
   IN  p_time    int,
   OUT p_known   int,
   OUT p_unknown int
)
SQL SECURITY DEFINER
BEGIN

  select count(*)
  into p_known
  from addresses a
  where a.ignore_addr = 1
    and timestampdiff(second, a.last_seen, now()) < p_time;
    
  select count(*)
  into p_unknown
  from addresses a
  where a.ignore_addr = 0
    and timestampdiff(second, a.last_seen, now()) < p_time;

END //
DELIMITER ;
