drop procedure if exists sp_web_prelogin;

/*
  Take username, and return salt. If username not known (or has no 
  password entry), return empty string.

*/

DELIMITER //
CREATE PROCEDURE sp_web_prelogin
(
   IN  username     varchar(100),
   OUT salt         varchar(40)
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;

  main: begin

    -- See if the username/handle is known
    select count(*) 
    into ck_exists
    from members m
    inner join members_auth ma on ma.member_id = m.member_id
    where upper(m.handle) = upper(username);

    if (ck_exists != 1) then
      set salt = ''; 
      leave main;
    end if;

    select
      ma.salt
    into
      salt
    from members m
    inner join members_auth ma on ma.member_id = m.member_id 
    where upper(m.handle) = upper(username);
    
  end main;
  

END //
DELIMITER ;


GRANT EXECUTE ON PROCEDURE sp_web_prelogin TO 'gk'@'localhost'
