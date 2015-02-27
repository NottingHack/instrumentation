<?php

include "dbaccess_header.php";

$sql = <<<SQL
  SELECT 
    COUNT(DISTINCT stats.last_day) AS last_day,
    COUNT(DISTINCT stats.last_week) AS last_week,
    COUNT(DISTINCT stats.last_month) AS last_month,
    COUNT(DISTINCT stats.last_quarter) AS last_quarter,
    COUNT(DISTINCT stats.last_year) AS last_year,
    COUNT(DISTINCT stats.anytime) AS anytime,
    (SELECT COUNT(*) FROM members WHERE members.member_status = 5) AS total_current_members

  FROM (

  -- Current members accessing the space
  SELECT
    IF(access_log.access_time >= DATE_SUB(NOW(), INTERVAL 1 DAY), members.member_id, NULL) AS last_day,  
    IF(access_log.access_time >= DATE_SUB(NOW(), INTERVAL 1 WEEK), members.member_id, NULL) AS last_week,
    IF(access_log.access_time >= DATE_SUB(NOW(), INTERVAL 1 MONTH), members.member_id, NULL) AS last_month,
    IF(access_log.access_time >= DATE_SUB(NOW(), INTERVAL 1 QUARTER), members.member_id, NULL) AS last_quarter,
    IF(access_log.access_time >= DATE_SUB(NOW(), INTERVAL 1 YEAR), members.member_id, NULL) AS last_year,
    members.member_id AS anytime
    
  FROM
    members
    JOIN access_log ON members.member_id = access_log.member_id
    JOIN
    (
      SELECT
        access_log.member_id,
        MAX(access_log.access_time) max_access_time
      FROM
        access_log
      GROUP BY
        access_log.member_id
    ) max_access_log ON
    access_log.member_id = max_access_log.member_id
    AND
    access_log.access_time = max_access_log.max_access_time
    
  WHERE
    members.member_status = 5

  UNION

  -- Current members using an access controlled tool
  SELECT
    IF(tl_tool_usages.usage_start >= DATE_SUB(NOW(), INTERVAL 1 DAY), members.member_id, NULL) AS last_day,  
    IF(tl_tool_usages.usage_start >= DATE_SUB(NOW(), INTERVAL 1 WEEK), members.member_id, NULL) AS last_week,
    IF(tl_tool_usages.usage_start >= DATE_SUB(NOW(), INTERVAL 1 MONTH), members.member_id, NULL) AS last_month,
    IF(tl_tool_usages.usage_start >= DATE_SUB(NOW(), INTERVAL 1 QUARTER), members.member_id, NULL) AS last_quarter,
    IF(tl_tool_usages.usage_start >= DATE_SUB(NOW(), INTERVAL 1 YEAR), members.member_id, NULL) AS last_year,
    members.member_id AS anytime
    
  FROM
    members
    JOIN tl_tool_usages ON members.member_id = tl_tool_usages.member_id
    JOIN
    (
      SELECT
        tl_tool_usages.member_id,
        MAX(tl_tool_usages.usage_start) max_usage_start
      FROM
        tl_tool_usages
      GROUP BY
        tl_tool_usages.member_id
    ) max_tl_tool_usages ON
    tl_tool_usages.member_id = max_tl_tool_usages.member_id
    AND
    tl_tool_usages.usage_start = max_tl_tool_usages.max_usage_start
    
  WHERE
    members.member_status = 5

  UNION

  -- Current members using the vending machine or note acceptor
  SELECT
    IF(vend_log.enq_datetime >= DATE_SUB(NOW(), INTERVAL 1 DAY), members.member_id, NULL) AS last_day,  
    IF(vend_log.enq_datetime >= DATE_SUB(NOW(), INTERVAL 1 WEEK), members.member_id, NULL) AS last_week,
    IF(vend_log.enq_datetime >= DATE_SUB(NOW(), INTERVAL 1 MONTH), members.member_id, NULL) AS last_month,
    IF(vend_log.enq_datetime >= DATE_SUB(NOW(), INTERVAL 1 QUARTER), members.member_id, NULL) AS last_quarter,
    IF(vend_log.enq_datetime >= DATE_SUB(NOW(), INTERVAL 1 YEAR), members.member_id, NULL) AS last_year,
    members.member_id AS anytime
    
  FROM
    members
    JOIN vend_log ON members.member_id = vend_log.member_id
    JOIN
    (
      SELECT
        vend_log.member_id,
        MAX(vend_log.enq_datetime) max_enq_datetime
      FROM
        vend_log
      GROUP BY
        vend_log.member_id
    ) max_vend_log ON
    vend_log.member_id = max_vend_log.member_id
    AND
    vend_log.enq_datetime = max_vend_log.max_enq_datetime
    
  WHERE
    members.member_status = 5

  ) stats;
SQL;

print sql_to_json(db_pdo_link(), $sql);

?>