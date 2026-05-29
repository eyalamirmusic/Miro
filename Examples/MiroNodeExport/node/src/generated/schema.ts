export interface User {
    id: string;
    name: string;
    isAdmin: boolean;
}

export interface Message {
    id: string;
    fromUserId: string;
    text: string;
    timestamp: number;
}

export type Severity = "Info" | "Warning" | "Error";

export interface GetUserRequest {
    id: string;
}

export interface ListUsersResponse {
    users: User[];
}

export interface SendMessageRequest {
    fromUserId: string;
    text: string;
}

export interface SendMessageResponse {
    message: Message;
}

export interface LogRequest {
    severity: Severity;
    message: string;
}

