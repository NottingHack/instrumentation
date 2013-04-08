drop procedure if exists sp_password_reset;

/*
 * Add password reset details
 */

DELIMITER //
CREATE PROCEDURE sp_password_reset
(
   IN  email      varchar(100),
   OUT handle     varchar(100),
   OUT member_id  int,
   OUT reset_id   int,  
   OUT keystr     varchar(40),  
   OUT err        varchar(100)   
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  set ck_exists = 0;
  set err = '';

  main: begin  
    
    -- check email address is valid
    select count(*) into ck_exists
    from members m
    where m.email = email;
  
    if (ck_exists != 1) then
      set err = "Email address not found";  
      leave main;
    end if;

    -- get member_id & handle
    select m.member_id, m.handle
    into member_id, handle
    from members m
    where m.email = email;
    
    -- Check there's not an outstanding password reset
    select count(*) into ck_exists
    from password_reset pr
    where pr.pr_status = 'PENDING'
      and pr.member_id = member_id;
  
    if (ck_exists = 1) then
      set err = "Password reset already sent";
      leave main;
    end if;

    -- Create new password reset
    select md5(rand())
    into keystr;    
    
    insert into password_reset (member_id, pr_key, pr_status)
    values (member_id, keystr, 'PENDING');
    
    set reset_id = last_insert_id();
 
  end main;

END //
DELIMITER ;
