drop procedure if exists sp_password_reset_validate;

/*
 * Check password reset id / key passed in exist in the db as 'pending'.
 */

DELIMITER //
CREATE PROCEDURE sp_password_reset_validate
(
   IN  reset_id   int,
   IN  keystr     varchar(40),
   OUT handle     varchar(100),
   OUT err        varchar(100)   
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  set ck_exists = 0;
  set err = '';
  set handle = '';

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

    -- Ok, get handle
    select m.handle
    into handle
    from password_reset pr
    inner join members m on m.member_id = pr.member_id
    where pr.reset_id = reset_id
      and pr.pr_key = keystr
      and pr.pr_status = 'PENDING';
 
  end main;

END //
DELIMITER ;
