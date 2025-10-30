// Format date time
export const formatDateTime = (dateTimeString: string) => {
  const date = new Date(dateTimeString);
  return date.toLocaleString('en-US', {
    month: 'short',
    day: 'numeric',
    year: 'numeric',
    hour: 'numeric',
    minute: 'numeric',
    hour12: true
  });
};

// Get status color
export const getStatusColor = (status: boolean) => {
  switch (status) {
    case true:
      return 'bg-green-100 text-green-800';
    case false:
      return 'bg-gray-100 text-gray-800';
    default:
      return 'bg-gray-100 text-gray-800';
  }
};

// Get charge level bar color
export const getChargeLevelColor = (level: number) => {
  if (level < 20) return 'bg-red-500';
  if (level < 50) return 'bg-amber-500';
  return 'bg-green-500';
};

// Get health color
export const getHealthColor = (health: number) => {
  if (health < 70) return 'text-red-500';
  if (health < 85) return 'text-amber-500';
  return 'text-green-500';
};

// Get temperature color
export const getTemperatureColor = (temp: number) => {
  if (temp > 45) return 'text-red-500';
  if (temp > 35) return 'text-amber-500';
  return 'text-gray-700';
};

// Get alert icon based on type
export const getAlertTypeColor = (type: string) => {
  switch (type) {
    case 'critical': return 'bg-red-100 text-red-800';
    case 'warning': return 'bg-amber-100 text-amber-800';
    case 'info': return 'bg-blue-100 text-blue-800';
    default: return 'bg-gray-100 text-gray-800';
  }
};


export function generate_random_string( length: number): string {
    const characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    let result = "";
    for (let i = 0; i < length; i++) {
        result += characters.charAt(Math.floor(Math.random() * characters.length));
    }
    return result;
}
