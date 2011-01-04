CREATE TABLE "nodetags" (
  "nodeid" INTEGER PRIMARY KEY NOT NULL,
  "key" TEXT NOT NULL ,
  "value" TEXT NOT NULL, 
  UNIQUE ( "nodeid","key") ON CONFLICT REPLACE
);