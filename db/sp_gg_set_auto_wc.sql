drop procedure if exists sp_gg_set_auto_wc;

DELIMITER //
CREATE PROCEDURE sp_gg_set_auto_wc
(
  IN ggemail_id       int,
  IN ggsum_auto_wc    int,
  IN ggemail_body_wc  varchar(10000)
)
SQL SECURITY DEFINER
BEGIN

  declare ck_exists int;
  declare ggemail_from varchar(200);
  declare ggemail_date timestamp;
  declare ggaddresses_id int;
  
  -- Update emails table with wordcounted markers
  select count(*)
  into ck_exists
  from gg_email ge
  where ge.ggemail_id = ggemail_id;
  
  if (ck_exists = 1) then
    update gg_email
    set gg_email.ggemail_body_wc = ggemail_body_wc
    where gg_email.ggemail_id = ggemail_id;
  end if;
  
  -- Update summary (set word count))
  select count(*)
  into ck_exists
  from gg_summary ggs
  where ggs.ggemail_id = ggemail_id;
  
  if (ck_exists = 1) then
    update gg_summary
    set gg_summary.ggsum_auto_wc = ggsum_auto_wc
    where gg_summary.ggemail_id = ggemail_id;
  else
  
    select ggm.ggemail_from, ggm.ggemail_date
    into ggemail_from, ggemail_date
    from gg_email ggm
    where ggm.ggemail_id = ggemail_id;
  
    if (ggemail_date is not null) then    
      call sp_gg_get_address_id(ggemail_from, ggaddresses_id);
  
      insert into gg_summary (ggemail_id, ggaddresses_id, ggsum_auto_wc, ggemail_date)
      select ggemail_id, ggaddresses_id, ggsum_auto_wc, ggemail_date;
    end if;
  end if;

END //
DELIMITER ;
