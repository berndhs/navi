CREATE TABLE "waytags" (
  "wayid" INTEGER PRIMARY KEY NOT NULL,
  "key" TEXT NOT NULL,
  "value" TEXT NOT NULL,
  PRIMARY KEY "wayid",
  UNIQUE ("wayid","key") ON CONFLICT REPLACE
);
