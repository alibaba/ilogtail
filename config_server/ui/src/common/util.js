import moment from "moment";

export const mapTimestamp = (ts) => (ts > 0 ? moment.unix(ts).format('YYYY-MM-DD HH:mm:ss') : '');