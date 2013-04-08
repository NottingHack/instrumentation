drop procedure if exists sp_web_login;


DELIMITER //
CREATE PROCEDURE sp_web_login
(
   IN  username     varchar(100),
   OUT ret          int,
   OUT member_id    int
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  declare m_member_id int;

  main: begin

    set ret = 0;
  
    -- See if the username/handle is known & member has permission to logon
    select count(*) 
    into ck_exists
    from members m
    where upper(m.username) = upper(username)
      and (fn_check_permission(m.member_id, 'WEB_LOGON')=1);

    if (ck_exists = 1) then
      select m.member_id
      into m_member_id
      from members m 
      where upper(m.username) = upper(username);

      set ret = 1;
      set member_id = m_member_id;
      leave main;
    else
      set ret = 0;
      leave main;
    end if;

  end main;
  

END //
DELIMITER ;
