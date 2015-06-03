<?php

include "dbaccess_header.php";

$sql = <<<SQL
  select 
    year(tu.usage_start) as Year, 
    monthname(tu.usage_start) as Month, 
    sec_to_time(sum(tu.usage_duration)) as "Time (hh:mm:ss)",
    (select count(*) from tl_members_tools mt where mt.tool_id=tu.tool_id and year(mt.mt_date_inducted)=year(tu.usage_start) and month(mt.mt_date_inducted)=month(tu.usage_start)) as "Members inducted"
  from tl_tool_usages tu
  where tu.tool_id = 1
    and tu.usage_duration > 0
  group by year(tu.usage_start),  monthname(tu.usage_start)
  order by year(tu.usage_start) desc,  month(tu.usage_start) desc
SQL;

print sql_to_tt_json(db_pdo_link(), $sql);

?>