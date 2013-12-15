drop procedure if exists sp_add_member;
/* err is 'added' on success, and member_id is the newly added members id */
DELIMITER //
CREATE PROCEDURE sp_add_member
(
   IN member_number   int,
   IN name            varchar(50),
   IN handle          varchar(100),
   IN unlock_text     varchar(95),
   IN enroll_pin      varchar(12),
   IN email           varchar(100),
   IN join_date       date,
   IN username        varchar(50),
   OUT err            varchar(100),
   OUT member_id      int
)
SQL SECURITY DEFINER
BEGIN

  declare ck_exists int;
  declare rowc int default 0; 
  set ck_exists = 1;
  set member_id = -1;
    
  main: begin
  
    if (enroll_pin = 'null') then
      set enroll_pin = null;
    end if;
    
    if (handle = 'null') then
      set handle = null;
    end if;    
    
    if (unlock_text = 'null') then
      set unlock_text = null;
    end if;      
    
    if (member_number is null) then
      set err = "Member number cannot be blank";
      leave main;
    end if;

    if (name is null) then
      set err = "Name cannot be blank";
      leave main;
    end if;

    select count(*) into ck_exists
    from members m
    where m.member_number = member_number;
    
    if (ck_exists > 0) then
      set err = "Member number already in database";
      leave main;
    end if;
    
    select count(*) into ck_exists
    from members m
    where m.name = name;
    
    if (ck_exists > 0) then
      set err = "Member name already in database";
      leave main;
    end if;    

    select count(*) into ck_exists
    from members m
    where m.handle = handle;
    
    if (ck_exists > 0) then
      set err = "Handle already in database";
      leave main;
    end if;

    if (username is not null) then
      select count(*) into ck_exists
      from members m
      where m.username = username;
      
      if (ck_exists > 0) then
        set err = "Username already in database";
        leave main;
      end if;
    end if;
    
    if (enroll_pin is not null) then
      select count(*) into ck_exists
      from pins
      where pin = enroll_pin
        and state in (10, 40); -- STATE_ACTIVE, STATE_ENROLL
    
      if (ck_exists > 0) then
        set err = "PIN already in use";
        leave main;
      end if;    
    end if;
      
    insert into members (member_number, name, handle, unlock_text, email, join_date, credit_limit, username) 
    values (member_number, name, handle, unlock_text, email, join_date, 5000, username); -- BAD / TODO: Find somewhere to store the default credit limit
    
    select row_count() into rowc;
    
    if (rowc != 1) then
      set err = 'Failed to add member (insert into members failed)';
    else
      set err = 'added';
      set member_id = last_insert_id();
    
      if (enroll_pin is not null) then
        insert into pins (pin, unlock_text, state, member_id)
        values (enroll_pin, 'N/A', 40, member_id); -- STATE_ENROLL
      end if;      
    end if;
    
  end main;

END //
DELIMITER ;
