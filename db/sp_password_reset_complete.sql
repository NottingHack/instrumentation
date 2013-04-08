drop procedure if exists sp_password_reset_complete;

/*
 * Complete password reset
 */

DELIMITER //
CREATE PROCEDURE sp_password_reset_complete
(
   IN  reset_id   int,
   IN  keystr     varchar(40),
   OUT username   varchar(50),
   OUT err        varchar(100)   
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  set ck_exists = 0;
  set err = '';

  main: begin  
    
    -- check reset_id / keystr are valid
    select count(*) into ck_exists
    from password_reset pr
    where pr.reset_id = reset_id
      and pr.pr_key = keystr
      and pr.pr_status = 'PENDING';
  
    if (ck_exists != 1) then
      set err = "Password reset details not valid";
      leave main;
    end if;
    
    select m.username
    into username
    from password_reset pr
    inner join members m on m.member_id = pr.member_id
    where pr.reset_id = reset_id
      and pr.pr_key = keystr
      and pr.pr_status = 'PENDING';

    -- Ok, update to completed
    update password_reset
    set 
      password_reset.pr_status = 'COMPLETED',
      password_reset.pr_completed = sysdate()
    where password_reset.reset_id = reset_id
      and password_reset.pr_key = keystr
      and password_reset.pr_status = 'PENDING';
 
  end main;

END //
DELIMITER ;
