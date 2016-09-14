drop procedure if exists sp_temperature_check;
/* err is null on success */
DELIMITER //
CREATE PROCEDURE sp_temperature_check
(
   OUT tlist varchar(1000)
)
SQL SECURITY DEFINER
BEGIN

  declare done int;
  declare tstr varchar(100);

  declare temperature_cur cursor for
    select  concat(t.name, ' : ',  cast(t.temperature as char(5)))
    FROM temperature t
    where t.name is not null
      and timestampdiff(minute,t.time, now()) < 20; -- ignore readings older than 20 minutes

    
  declare continue HANDLER for not found set done = true;

  open temperature_cur;

  set done = false;
  set tlist = null;

  read_loop: loop

    fetch temperature_cur INTO tstr;

    if done then 
      leave read_loop;
    end if;

    if (tlist is null) then
      set tlist = tstr;
    else
      set tlist = concat(tlist, ", ",  tstr);
    end if;

  end loop;

  close temperature_cur;  

END //
DELIMITER ;


